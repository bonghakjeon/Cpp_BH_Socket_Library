// BH_Socket.cpp : 정적 라이브러리를 위한 함수를 정의합니다.


// #include "framework.h"
#include "BH_Socket.h" 

using namespace std;

// BH_EchManager 클래스의 객체 생성자 BH_EchManager
BH_EchManager::BH_EchManager()
{
    mp_data = NULL;
    m_tot_size = 0;
    m_cur_size = 0;
}

// BH_EchManager 클래스의 객체 소멸자 ~BH_EchManager
BH_EchManager::~BH_EchManager()
{
    DelData();     // 전송 또는 수신에서 사용되던 메모리를 제거한다.
}

// 전송 또는 수신에 사용할 메모리를 할당하는 함수 MemoryAlloc의 정의
char* BH_EchManager::MemoryAlloc(int a_data_size)
{
    /* 기존에 사용하던 메모리와 현재 필요한 메모리가 크기가 동일하다면
       다시 메모리를 할당할 필요가 없다.
       (일정한 크기의 데이터를 지속적으로 전송하거나 수신하는 경우가 많다.) */
    if (m_tot_size != a_data_size)
    {
        // 이미 할당된 메모리가 있다면 제거함.
        if (mp_data != NULL) delete[] mp_data;

        // 시정한 크기로 메모리를 할당함.
        mp_data = new char[a_data_size];

        // 할당된 크기를 기억한다.
        m_tot_size = a_data_size;
    }   // End of if

    // 작업 위치를 가장 첫 위치로 초기화함.
    m_cur_size = 0;

    // 할당된 메모리의 시작 위치를 반환함.
    return mp_data;
}


// 전송 또는 수신에서 사용되던 메모리를 제거하는 DelData 함수의 정의
void BH_EchManager::DelData()
{
    if (mp_data != NULL)
    {
        // 할당된 메모리를 제거하고 작업과 관련된 변수들을 초기화 함.
        delete[] mp_data;
        mp_data = NULL;
        m_tot_size = 0;
    }
}

// BH_SendManager 클래스에서 현재 전송할 위치와 크기를 계산하는 함수 GetPos의 정의
int BH_SendManager::GetPos(char** ap_data, int a_data_size)
{
    // 새로운 전송 위치에 대한 주소를 첫 번째 인자에 저장함.
    *ap_data = mp_data + m_cur_size;

    // 전송 크기를 계산하기 위해서 2048 bytes를 더한 크기가 최대 크기보다 작은지 체크함.
    if (m_cur_size + a_data_size < m_tot_size)
    {
        /* 최대 크기보다 작은 경우 2048 bytes만큼 전송하면 됨.
           다음 위치를 계산할 수 있도록 전송한 총 크기를 구한다. */
        m_cur_size += a_data_size;
    }
    else
    {
        // 2048 bytes 보다 작은 경우, 실제로 남은 크기만 전송함.
        a_data_size = m_tot_size - m_cur_size;

        // 현재 위치를 마지막 위치로 옮김. (이번이 마지막 전송임.)
        m_cur_size = m_tot_size;
    }

    // 계산된 전송 크기를 반환함.
    return a_data_size;
}

/* BH_RecvManager 클래스에서 수신된 데이터를 기존에 수신 되어 있는 데이터에 추가하는
   AddData 함수의 정의 */
int BH_RecvManager::AddData(char* ap_data, int a_size)
{
    // 수신된 데이터를 기존에 수신된 데이터 뒤에 추가함.
    memcpy(mp_data + m_cur_size, ap_data, a_size);

    // 총 수신된 데이터의 크기를 계산함.
    m_cur_size += a_size;

    // 현재 수신된 데이터의 크기를 반환함.
    return m_cur_size;
}

// BH_Socket 클래스의 객체 생성자 BH_Socket
BH_Socket::BH_Socket(unsigned char a_val_key, int a_data_notify_id)
{
    // 사용자가 지정한 프로토콜 구분 값을 저장함.
    m_val_key = a_val_key;

    // 전송용으로 사용할 8 Kbytes 메모리를 할당함.
    mp_send_data = new char[8192];

    // 자신이 전송할 프레임의 선두 바이트에 구분 값을 미리 넣어둠.
    /* unsigned char 포인터 형으로 형변환한 후 mp_send_data 포인터가
       a_val_key를 가리키게 된다. */
    *(unsigned char*)mp_send_data = a_val_key; // default

    // 수신용으로 사용할 8 Kbytes 메모리를 할당함.
    mp_recv_data = new char[8192];

    // 데이터 수신 및 연결 해제 메시지를 받을 윈도우 핸들 값을 초기화 함.
    mh_notify_wnd = NULL;

    /* 데이터 수신 및 연결 해제 메시지 ID로 사용할 값을 사용자에게 넘겨받아 저장함. */
    m_data_notify_id = a_data_notify_id;

    // 소켓 시스템을 사용하도록 설정함.
    WSADATA temp;
    WSAStartup(0x0202, &temp);
}

// BH_Socket 클래스의 객체 소멸자 ~BH_Socket
BH_Socket::~BH_Socket()
{
    // 전송과 수신에 사용하던 메모리를 제거함.
    delete[] mp_send_data; // 전송에 사용하던 메모리
    delete[] mp_recv_data; // 수신에 사용하던 메모리

    // 소켓 시스템을 더 이상 사용하지 않도록 설정함.
    WSACleanup();
}

// 연결된 대상에게 네트워크로 데이터를 전송할 때 사용할 함수 SendFramData의 정의
int BH_Socket::SendFrameData(SOCKET ah_socket, unsigned char a_msg_id, const char* ap_body_data, BSize a_body_size)
{
    /* 메시지 ID를 두 번째 바이트에 저장함.
       첫 번째 바이트에는 구분값이 이미 들어있음.(객체 생성자에서 작업함.)
       *(unsigned char*)mp_send_data = a_val_key; */
    *(unsigned char*)(mp_send_data + 1) = a_msg_id;

    /* 세 번째와 네 번째 바이트에 'Body' 데이터의 크기를 저장함. (현재 2 bytes 크기임.)
       BSize는 unsigned short int 자료형을 의미함. */
    *(BSize*)(mp_send_data + 2) = a_body_size;

    /* 다섯 번째 위치에 'Body'를 구성하는 데이터를 복사함.
       HEAD_SIZE는 2+sizeof(BSize)를 의미함. */
    memcpy(mp_send_data + HEAD_SIZE, ap_body_data, a_body_size);

    /* 소켓으로 데이터를 전송함.
       전송하는 총량은 Head와 Body의 크기를 합산한 것과 같음. */
    if (send(ah_socket, mp_send_data, HEAD_SIZE + a_body_size, 0) == (a_body_size + HEAD_SIZE))
    {
        return 1;   // 전송에 성공함.
    }

    return 0;       // 전송에 실패함.  
}


// 재시도 수신을 지원하는 함수 ReceiveData의 정의
int BH_Socket::ReceiveData(SOCKET ah_socket, BSize a_body_size)
{
    BSize tot_size = 0, retry = 0;
    int read_size;

    /* 소켓의 수신이 느려서 a_body_size에 지정한 크기만큼 한번에 수신하지
       못 할수 있기 때문에 반복문을 이용해서 여러번 재시도 하면서 읽는다. */
    while (tot_size < a_body_size)
    {
        /* 처음에는 a_body_size만큼 읽기를 시도하지만 한번에 다 못읽은 경우
           실제로 읽은 크기를 제외한 나머지 크기를 계속 반복해서 읽음. */
        read_size = recv(ah_socket, mp_recv_data + tot_size, a_body_size - tot_size, 0);

        if (read_size == SOCKET_ERROR)
        {
            /* 데이터 수신도중 소켓에 오류가 난 경우에 0.5초동안
               10회 반복하면서 재시도한다. */
            retry++;
            Sleep(50);   // 0.5초 동안 지연 시킴.

            if (retry > 10) break;
            /* rerty가 10보다 큰 경우 while문을 탈출하여 소켓에 오류가 발생하여
               수신을 실패 했음을 알린다. */
        }
        else   // 정상적으로 데이터를 수신하는 경우
        {
            // 실제로 수신된 크기를 tot_size 변수에 합산함.
            if (read_size > 0)
            {
                tot_size = (BSize)(tot_size + read_size);
            }

            /* 총 수신된 크기가 아직 a_body_size보다 작다면 수신 상태가 좋지 않다는
               뜻이기 때문에 약간의 지연을 주면서 다시 읽기를 시도하는 것이 좋음. */
            if (tot_size < a_body_size)
            {
                Sleep(5);  // 0.05초 동안 지연 시킴.
            }

            /* 중간에 오류가 나서 재시도 했더라도 다시 수신을 하기 시작했다면
               재시도 횟수를 초기화함. */
            retry = 0;
        }
    }
    return retry <= 10;
    // retry의 값이 0을 반환하면 수신 실패, 1을 반환하면 수신 성공을 의미함.
}

// 데이터가 수신되었을 때 수신된 데이터를 처리하는 함수 ProcessRecvEvent의 정의
void BH_Socket::ProcessRecvEvent(SOCKET ah_socket)
{
    unsigned char msg_id;
    BSize body_size;

    // FD_READ 이벤트가 과도하게 발생하지 않도록 FD_READ 이벤트를 제거함.
    ::WSAAsyncSelect(ah_socket, mh_notify_wnd, m_data_notify_id, FD_CLOSE);

    unsigned char key = 0;

    // 구분 값을 수신함.
    recv(ah_socket, (char*)&key, 1, 0);

    // 사용자가 지정한 구분 값과 일치하는지 체크함.
    if (key == m_val_key)
    {
        // Message ID를 수신함. (1 byte)
        recv(ah_socket, (char*)&msg_id, 1, 0);

        // Body size를 수신함. (2 bytes)
        recv(ah_socket, (char*)&body_size, sizeof(BSize), 0);

        if (body_size > 0)
        {
            /* Body size가 0인 경우에는 수신할 필요가 없음.
               Body size는 크기 때문에 recv 함수가 아닌 안정적인 재시도 수신이
               가능한 ReceiveData 함수로 데이터를 수신함. */

               // ReceiveData 함수로 데이터를 수신함.
            if (!ReceiveData(ah_socket, body_size))
                /* ReceiveData가 0을 반환한 경우 retry의 값이 0을 반환했음을
                   의미하므로 수신을 실패했다는 것을 알 수 있고,
                   0의 부정은 양수(혹은 1)을 의미하므로 if문을 실행하게 됨. */
            {
                /* 데이터를 수신하다가 오류가 발생한 경우 연결된 소켓을 해제함. */
                DisconnSocket(ah_socket, -2);

                /* return; 명령을 사용하여
                   void형 함수 ProcessRecvEvent를 실행 도중에 강제로 수행을 종료함. */
                return;
            }
        }

        /*  정상적으로 'Head'와 'Body'를 수신한 경우에는 이 정보들을 사용하여
            사용자가 원하는 작업을 처리함.
            ProcessRecvData 함수는 상속 받은 자식 클래스에서 오버라이딩 기술로
            재정의하여 자신들이 원하는 작업을 추가하는 함수임. */
        if (1 == ProcessRecvData(ah_socket, msg_id, mp_recv_data, body_size))
        {
            /* 소켓에 문제가 발생하지 않았다면 다시 수신 이벤트 처리가 가능하도록
               FD_READ 옵션을 추가해 줌. */
            ::WSAAsyncSelect(ah_socket, mh_notify_wnd, m_data_notify_id, FD_CLOSE | FD_READ);
        }
    }
    else DisconnSocket(ah_socket, -1);
    // 구분값이 잘못된 경우, 접속을 해제함.
}

// 접속된 대상을 끊을 때 사용하는 함수 DisconnSocket - 자식 클래스에서 꼭 재정의해서 사용해야 함.
void BH_Socket::DisconnSocket(SOCKET ah_socket, int a_err_code)
{

}

// 수신된 데이터를 처리하는 함수 ProcessRecvData - 자식 클래스에서 꼭 재정의해서 사용해야 함.
int BH_Socket::ProcessRecvData(SOCKET ah_socket, unsigned char a_msg_id, char* ap_recv_data, BSize a_body_size)
{
    return 0;
}

// ASCII 형식의 문자열을 유니코드로 변환하는 함수 AscToUnic의 정의
void BH_Socket::AscToUnic(wchar_t* ap_dest_ip, char* ap_src_ip)
{
    // ASCII 형식의 문자열을 유니코드로 변환함.
    int ip_length = strlen(ap_src_ip) + 1;

    /* memset 함수는 어떤 메모리의 시작점부터 연속된 범위를 어떤 값으로(바이트 단위)
       모두 지정하고 싶을 때 사용하는 함수이다.

       memset 함수의 기본 구조

       void * memset(void * ptr, int value, size_t num);

       ptr : 채우고자 하는 메모리의 시작 포인터(시작주소)
       value : 메모리에 채우고자 하는 값이다.
               int 형이지만 내부에서는 unsigned char (1 byte)로 변환되어서 저장됨.
       num : 채우고자 하는 바이트의 수이다.
             즉, 채우고자 하는 메모리의 크기를 의미함. */

             /* 시프트 연산은 변수 << 이동할 비트 수 또는 변수 >> 이동할 비트 수 형식으로 사용함.
                즉 지정한 횟수대로 비트를 이동시키며 모자라는 공간은 0으로 채운다.
                연산자 모양 그대로 << 는 왼쪽, >> 는 오른쪽 방향이다.
                그리고 << 는 변수에 저장된 값에 2의 거듭제곱을 곱하는 것이고,
                >> 는 2의 거듭제곱을 나누는 것이다.
                현재 ip_length << 1 이 의미하는 것은 ip_length * 2^1을 의미한다. */
    memset(ap_dest_ip, 0, ip_length << 1);

    // 1 바이트 형식으로 되어 있는 문자열(ap_src_ip)을 2 바이트 형식으로 변경하면 됨.
    for (int i = 0; i < ip_length; i++)
        ap_dest_ip[i] = ap_src_ip[i];
}

// 유니코드 형식의 문자열을 ASCII로 변환하는 함수 UnicToAsc의 정의
void BH_Socket::UnicToAsc(char* ap_dest_ip, wchar_t* ap_src_ip)
{
    /* wchar_t 는 와이드 문자(wide character)를 저장하기 위한 자료형이다.
       보통 영문 알파벳은 1바이트로 표현하지만 유니코드는 2바이트로 표현하기
       때문에 wchar_t에 저장해야 한다.
       그리고 wchar_t 문자와 문자열은 아래와 같이 따옴표 안에 L을 붙여서 표현한다.

       (ex) wchar_t wc = L'a';

            wchar_t * ws1 = L"Hello, world!";

            wchar_t * ws2 = L"안녕하세요.";

       그리고 wchar_t 문자열은 w가 붙은 함수를 사용해야 하고,
       이 함수들은 wchar.h 헤더파일에 선언되어 있다. */

       /* 유니코드 형식의 문자열을 ASCII로 변환함.

          wcslen 함수는 string에서 가리키는 스트링에서 와이드 문자(wchar_t) 개수를 계산함.
          wcslen 함수는 실행을 종료하면 wchar_t 널 문자를 제외하고 string에서
          와이드 문자(wchar_t) 개수를 리턴함. */
          // 1을 더하는 이유는 1이 널문자를 의미하기 때문이다.
    int ip_length = wcslen(ap_src_ip) + 1;

    // 2 바이트 형식으로 되어 있는 문자열(ap_src_ip)을 1 바이트 형식으로 변경하면 됨.
    for (int i = 0; i < ip_length; i++)
    {
        ap_dest_ip[i] = (char)ap_src_ip[i];
    }
}

// BH_UserData 클래스의 객체 생성자 BH_UserData
BH_UserData::BH_UserData()
{
    mh_socket = INVALID_SOCKET;         // 소켓 핸들 초기화
    m_ip_addr[0] = 0;                   // 주소 값 초기화
    mp_send_man = new BH_SendManager;   // 전송용 객체 생성
    mp_recv_man = new BH_RecvManager;   // 수신용 객체 생성
}

// BH_UserData 클래스의 객체 소멸자 ~BH_UserData
BH_UserData::~BH_UserData()
{
    // 소켓이 생성되어 있다면 소켓을 제거한다.
    if (mh_socket != INVALID_SOCKET)
    {
        closesocket(mh_socket);
    }

    // 전송과 수신을 위해 생성했던 객체를 제거함.
    delete mp_send_man;   // 전송용 객체 제거    
    delete mp_recv_man;   // 수신용 객체 제거
}

// 클라이언트의 IP 주소를 얻는 함수 GetIP의 정의
wchar_t* BH_UserData::GetIP()
{
    return m_ip_addr;
}

// 클라이언트의 IP 주소를 설정하는 함수 SetIP의 정의
void BH_UserData::SetIP(const wchar_t* ap_ip_addr)
{
    /* wcscpy 함수의 기본형
       wchar_t *wcscpy(wchar_t * string1, const wchar_t * string2);

       wcscpy 함수는 매개변수 string2(wchar_t의 널 문자 포함)를
       string1에 복사하는 기능을 가진 함수이다.

       wcscpy 함수는 널로 종료되는 wchar_t 스트링에서 작동한다.
       이 함수에 대한 스트링 인수는 스트링의 끝을 나타내는 널 문자를
       포함해야 한다. 하지만 string1, string2 중에서 string2만
       널 문자를 포함해야 한다.
       그리고 wcscpy 함수는 string1에 대한 포인터를 리턴한다. */
    wcscpy(m_ip_addr, ap_ip_addr);
}

// 소켓을 닫고 초기화하는 함수 CloseSocket의 정의
void BH_UserData::CloseSocket(char a_linger_flag)
{
    /* 소켓 정보가 할당되어 있다면 소켓을 닫고 초기화함.
       a_linger_flag에 1을 명시하면 소켓이 데이터를 수신 중이더라도
       즉시 소켓을 제거한다. */
    if (mh_socket != INVALID_SOCKET)
    {
        if (a_linger_flag)
        {
            LINGER temp_linger = { TRUE, 0 };
            /* 링거 옵션을 변경하여 소켓이 데이터를 수신중이더라도 소켓을
               즉시 제거할 수 있음. */

            setsockopt(mh_socket, SOL_SOCKET, SO_LINGER, (char*)&temp_linger, sizeof(temp_linger));
        }

        // 소켓이 생성되어 있다면 소켓을 제거한다.
        closesocket(mh_socket);

        mh_socket = INVALID_SOCKET;   // 소켓 핸들 초기화
    }
}

// BH_ServerSocket 클래스의 객체 생성자 BH_ServerSocket
/* 매개변수로 전달되는 값 중에 a_val_key와 a_data_notify_id는 BH_Socket 클래스에서
   관리하는 데이터이기 때문에 부모 클래스(BH_Socket)의 객체 생성자를 호출하여 전달함. */
BH_ServerSocket::BH_ServerSocket(unsigned char a_val_key, unsigned short int a_max_user_cnt, BH_UserData* ap_user_data,
    int a_accept_notify_id, int a_data_notify_id) : BH_Socket(a_val_key, a_data_notify_id)
{
    // 서버에 접속할 최대 사용자 수를 저장함.
    m_max_user_cnt = a_max_user_cnt;

    // Listen 작업용 소켓을 초기화함.
    mh_listen_socket = INVALID_SOCKET;

    // 최대 사용자 수만큼 사용자 정보를 저장할 객체를 관리할 포인터를 생성함.
    mp_user_list = new BH_UserData * [m_max_user_cnt];

    // 최대 사용자 수만큼 사용자 정보를 저장할 객체를 개별적으로 생성함.
    for (int i = 0; i < m_max_user_cnt; i++)
    {
        /* 다형성 사용 시에 특정 클래스 형식에 종속되지 않도록 멤버 함수를
           사용해서 객체를 생성함.

           CreateObj 함수는 매개변수로 전달된 ap_user_data와 동일한 객체를 생성함. */
        mp_user_list[i] = ap_user_data->CreateObj();
    }

    // 새로운 사용자가 접속했을 때 발생할 메시지 ID를 저장함.
    m_accept_notify_id = a_accept_notify_id;

    /* 사용자 정보를 생성하기 위해 매개변수로 전달 받은 객체를 제거함.
       이미 동일한 형식의 객체가 mp_user_list에 할당되어 있기 때문에
       보관할 필요가 없음. */
    delete ap_user_data;
}

// BH_ServerSocket 클래스의 객체 소멸자 ~BH_ServerSocket
BH_ServerSocket::~BH_ServerSocket()
{
    // listen 소켓이 생성되어 있다면 제거함.
    if (mh_listen_socket != INVALID_SOCKET)
    {
        closesocket(mh_listen_socket);
    }

    // 최대 사용자 수만큼 생성되어 있던 객체를 제거함.
    for (int i = 0; i < m_max_user_cnt; i++)
    {
        delete mp_user_list[i];
    }

    // 사용자 객체를 관리하기 위해 생성했던 포인터를 제거함.
    delete[] mp_user_list;
}

// 서버 서비스를 시작하는 함수 StartServer의 정의
int BH_ServerSocket::StartServer(const wchar_t* ap_ip_addr, int a_port, HWND ah_notify_wnd)
{
    /* 반환값이 1이면 소켓 생성 성공, -1이면 소켓 생성 실패,
       -2이면 생성된 소켓을 바인드 하는데 실패를 의미함. */

       /* 비동기 형식의 소켓에 이벤트(FD_ACCEPT, FD_READ, FD_CLOSE)가 발생했을 때
          전달되는 메시지를 수신할 윈도우의 핸들을 저장함. */
    mh_notify_wnd = ah_notify_wnd;

    struct sockaddr_in serv_addr;

    /* listen 작업용 소켓을 TCP 형식으로 생성함.
       SOCK_STREAM이 TCP를 의미함. */
    mh_listen_socket = socket(AF_INET, SOCK_STREAM, 0);

    // 소켓 생성에 성공했는지 체크함.
    if (mh_listen_socket < 0)
    {
        return -1;   // 소켓 생성 실패함!!
    }

    char temp_ip_addr[16];

    // 유니코드 형식으로 전달된 IP 주소를 ASCII 형식으로 변경함.
    UnicToAsc(temp_ip_addr, (wchar_t*)ap_ip_addr);

    memset((char*)&serv_addr, 0, sizeof(serv_addr));

    /* 네트워크 장치에 mh_listen_socket을 연결하기 위해서
       IP 주소와 포트번호를 가지고 기본 정보를 구성함. */
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(temp_ip_addr); // INADDR_ANY;
    serv_addr.sin_port = htons((short unsigned int)a_port);

    /* NIC란?
       네트워크 인터페이스 컨트롤러(Network Interface Controller)의 약자이며,
       컴퓨터를 네트워크에 연결하여 통신하기 위해 사용하는 하드웨어 장치이다.
       네트워크 카드(Network Card), 랜 카드(LAN card), 물리 네트워크 인터페이스
       (physical network interface)라고 하며, 네트워크 인터페이스 카드,
       네트워크 어댑터, 네트워크 카드, 이더넷 카드 등으로도 불린다. */

       /* INADDR_ANY의 의미?
          INADDR_ANY는 서버의 IP주소를 자동으로 찾아서 대입해주는 함수이다.

          INADDR_ANY 지정할 경우에 생기는 이점(2 가지)

          첫 번째, 멀티 네트워크 카드가 동시에 지원된다.
          서버는 NIC를 2개 이상 가지고 있는 경우가 많은데 만일 특정 NIC의 IP주소를
          sin_addr.s_addr에 지정하면 다른 NIC에서 요청된 연결은 서비스 할 수 없게 됨.
          이 때, INADDR_ANY를 사용하면 두 NIC를 모두 바인딩 해주므로 어느 IP를 통해
          접속하더라도 정상적인 서비스가 가능함.

          두 번째, 이식성이 편리하다.
          또 다른 이점은 이식성인데, 특정 IP를 지정했을 경우 다른 서버 컴퓨터에
          프로그램이 설치된다면 주소값을 변경(소스 수정)해야 하지만,
          INADDR_ANY를 사용하면 소스 수정 없이 곧바로 사용 또는 컴파일 할 수 있다.*/

          // 네트워크 장치에 mh_listen_socket 소켓을 연결함.
    if (bind(mh_listen_socket, (struct sockaddr*) & serv_addr, sizeof(serv_addr)) < 0)
    {
        // 소켓 연결에 실패한 경우 소켓을 제거하고 mh_listen_socket 변수를 초기화함.
        closesocket(mh_listen_socket);
        mh_listen_socket = INVALID_SOCKET;

        return -2;  // bind 오류가 발생함을 의미함.
    }

    // 클라이언트 접속을 허락함. 이 시점부터 클라이언트의 접속이 가능해짐.
    listen(mh_listen_socket, 5);

    /* 새로운 클라이언트가 접속 했을 때 발생하는 FD_ACCEPT 이벤트를 체크해서
       ah_notify_wnd에 해당하는 윈도우로 m_accept_notify_id 메시지를 전달하는
       비동기를 설정함.

       accept 함수를 바로 호출하면 새로운 사용자가 접속할 때까지 프로그램이
       응답 없음에 빠지기 때문에 비동기 설정을 사용하는 것임. */
    WSAAsyncSelect(mh_listen_socket, ah_notify_wnd, m_accept_notify_id, FD_ACCEPT);

    return 1;   // listen 작업이 성공함을 의미함. 
}

// 새로운 클라이언트가 접속할 때 사용할 함수 ProcessToAccpet의 정의
int BH_ServerSocket::ProcessToAccept(WPARAM wParam, LPARAM lParam)
{
    /* - FD_ACCEPT와 연결된 메시지가 발생했을 때 사용함.
       함수의 반환값이 1이면 accept 성공, -1이면 accept 실패,
       -2이면 접속 가능한 최대 사용자 수 초과를 의미함. */
    struct sockaddr_in cli_addr;
    int temp_client_info_size = sizeof(cli_addr), i;

    // 새로 접속을 시도하는 클라이언트와 통신할 소켓을 생성함.
    SOCKET h_client_socket = accept((SOCKET)wParam, (struct sockaddr*) & cli_addr, &temp_client_info_size);

    // 소켓 생성이 실패한 경우
    if (h_client_socket == INVALID_SOCKET)
    {
        return -1;
    }
    else   // 소켓 생성에 성공한 경우
    {
        BH_UserData* p_user;
        wchar_t temp_ip_addr[16];

        // 새로 접속한 클라이언트의 IP를 유니코드 형식의 문자열로 변환함.
        AscToUnic(temp_ip_addr, inet_ntoa(cli_addr.sin_addr));

        /* 사용자 정보를 저장할 객체들 중에 아직 소켓을 배정받지 않은
           객체를 찾아서 현재 접속한 사용자 정보를 저장함. */
        for (i = 0; i < m_max_user_cnt; i++)
        {
            p_user = mp_user_list[i];

            // 사용자 정보의 소켓 핸들 값이 INVALID_SOCKET이라면 해당 객체는 미사용 중인 객체이다.
            if (p_user->GetHandle() == INVALID_SOCKET)
            {
                // 사용자 정보에 소켓을 저장함.
                p_user->SetHandle(h_client_socket);

                // 사용자 정보에 IP 주소를 저장함.
                p_user->SetIP(temp_ip_addr);

                /* 연결된 클라이언트가 데이터를 전송(FD_READ)하거나 연결을 해제(FD_CLOSE)하면
                   mh_notify_wnd 핸들 값에 해당하는 윈도우로 m_data_notify_id에 저장된
                   메시지 ID가 전달되도록 비동기를 설정함. */
                WSAAsyncSelect(h_client_socket, mh_notify_wnd, m_data_notify_id, FD_READ | FD_CLOSE);

                /* 새로운 사용자가 접속한 시점에 처리해야할 작업을 한다.
                   (클래스 사용자가 직접 구현해야 함.) */
                AddWorkForAccept(p_user);
                break;
            }   // End of if
        }   // End of for

        // 최대 접속자 수를 초과하여 더 이상 클라이언트의 접속을 허락할 수 없는 경우
        if (i == m_max_user_cnt)
        {
            // 접속자 수 초과에 대한 작업을 진행함. (클래스 사용자가 직접 구현해야 함.)
            ShowLimitErr(temp_ip_addr);

            // 접속한 소켓을 제거함.
            closesocket(h_client_socket);

            return -2; // 사용자가 초과 했음을 의미함.
        }   // End of if
    }   // End of else

    return 1; // 정상적으로 접속을 처리했음을 의미함.
}

/* 새로운 데이터가 수신(FD_READ)되거나 클라이언트가 연결이 해제(FD_CLOSE)될 때
   발생하는 메시지에서 서버 소켓과 관련된 작업을 처리해주는 함수 ProcessClientEvent의 정의 */
void BH_ServerSocket::ProcessClientEvent(WPARAM wParam, LPARAM lParam)
{
    // 클라이언트가 데이터를 전송한 경우
    if (WSAGETSELECTEVENT(lParam) == FD_READ)
    {
        // 전송된 데이터를 분석해서 처리하는 함수를 호출함.
        ProcessRecvEvent((SOCKET)wParam);

    }
    else // FD_CLOSE, 클라이언트가 접속을 해제한 경우
    {
        BH_UserData* p_data = FindUserData((SOCKET)wParam);

        // 클라이언트가 접속을 해제할 시 추가로 작업할 내용을 수행함.
        AddWorkForCloseUser(p_data, 0);

        // 클라이언트가 접속을 해제한 소켓을 닫고 초기화함.
        p_data->CloseSocket(0);
    }
}

// 클라이언트가 접속을 해제할 시 추가로 작업할 내용을 수행하는 함수 AddWorkForCloseUser의 정의
void BH_ServerSocket::AddWorkForCloseUser(BH_UserData* ap_user, int a_err_code)
{
    /* 클라이언트가 접속 해제 시에 추가적으로 해야할 작업이 있다면
      이 함수를 오버라이딩해서 처리해야함.
      a_err_code : 0이면 정상 종료, -1이면 키값이 유효하지 않아서 종료,
                   -2이면 바디 정보 수신 중에 오류가 발생함을 의미함. */
}

// ah_socket 핸들 값을 사용하는 소켓 사용자를 강제로 종료 시키는 함수 DisconnSocket의 정의
void BH_ServerSocket::DisconnSocket(SOCKET ah_socket, int a_err_code)
{
    // 소켓 핸들을 사용하여 사용자 정보를 찾음.
    BH_UserData* p_user_data = FindUserData(ah_socket);

    // 접속을 해제하기 전에 작업해야 할 내용을 처리함.
    AddWorkForCloseUser(p_user_data, a_err_code);

    // 해당 사용자의 소켓을 닫음.
    p_user_data->CloseSocket(1);
}

// FD_READ 이벤트가 발생했을 때 실제 데이터를 처리하는 함수 ProcessRecvData의 정의
int BH_ServerSocket::ProcessRecvData(SOCKET ah_socket, unsigned char a_msg_id, char* ap_recv_data,
    BSize a_body_size)
{
    // 소켓 핸들 값을 사용하여 이 소켓을 사용하는 사용자를 찾음.
    BH_UserData* p_user_data = FindUserData(ah_socket);

    // 예약 메시지 251번은 클라이언트에 큰 용량의 데이터를 전송하기 위해 사용함.
    if (a_msg_id == 251)
    {
        char* p_send_data;

        // 현재 전송 위치를 얻음.
        BSize send_size = p_user_data->GetSendMan()->GetPos(&p_send_data);

        /* 전송할 데이터가 더 있다면 예약 메시지 번호인 252를 사용하여
           클라이언트에게 데이터를 전송함. */
        if (p_user_data->GetSendMan()->IsProcessing())
        {
            SendFrameData(ah_socket, 252, p_send_data, send_size);
        }
        else
        {
            /* 지금이 분할된 데이터의 마지막 부분이라면 예약 메시지 번호인 253번을
               사용하여 클라이언트에게 데이터를 전송함. */
            SendFrameData(ah_socket, 253, p_send_data, send_size);

            // 마지막 데이터를 전송하고 전송에 사용했던 메모리를 삭제함.
            p_user_data->GetSendMan()->DelData();

            /* 서버 소켓을 사용하는 윈도우에 전송이 완료 되었음을 알려줌.
               전송이 완료 되었을 때 프로그램에 어떤 표시를 하고 싶다면 해당 윈도우에서
               LM_SEND_COMPLTED 메시지를 체크하면 됨. */
            ::PostMessage(mh_notify_wnd, LM_SEND_COMPLETED, (WPARAM)p_user_data, 0);
        }
    }   // End of if
    else if (a_msg_id == 252)
    {
        /* 252번은 대용량의 데이터를 수신할 때 사용하는 예약된 메시지 번호임.
           수신된 데이터는 수신을 관리하는 객체로 넘겨서 데이터를 합침. */
        p_user_data->GetRecvMan()->AddData(ap_recv_data, a_body_size);

        /* 252번은 아직도 추가로 수신할 데이터가 있다는 뜻이기 때문에
           예약 메시지 251번을 클라이언트에 전송하여 추가 데이터를 요청함. */
        SendFrameData(ah_socket, 251, NULL, 0);
    }
    else if (a_msg_id == 253)
    {
        /* 253번은 대용량의 데이터를 수신할 때 사용하는 예약된 메시지 번호임.
           수신된 데이터는 수신을 관리하는 객체로 넘겨서 데이터를 합침. */
        p_user_data->GetRecvMan()->AddData(ap_recv_data, a_body_size);

        /* 253번은 데이터 수신이 완료되었다는 메시지이기 때문에 서버 소켓을 사용하는
           윈도우에 완료 되었음을 알려줌.

           따라서 윈도우에서 수신이 완료된 데이터를 사용하려면 LM_RECV_COMPLETED
           메시지를 사용하면 됨.

           LM_RECV_COMPLETED 메시지를 수신한 처리기에서 수신할 때 사용한 메모리를
           DelData 함수를 호출해서 제거해야 함. */
        ::PostMessage(mh_notify_wnd, LM_RECV_COMPLETED, (WPARAM)p_user_data, 0);
    }

    /* 수신된 데이터를 정상적으로 처리함.
       만약, 수신 데이터를 처리하던 중에 소켓을 제거했으면 0을 반환해야 함.
       0을 반환하면 이 소켓에 대해서 비동기 작업이 중단됨. */
    return 1;
}

// BH_ClientSocket 클래스의 객체 생성자 BH_ClientSocket
BH_ClientSocket::BH_ClientSocket(unsigned char a_val_key, int a_conn_notify_id,
    int a_data_notify_id) : BH_Socket(a_val_key, a_data_notify_id)
{
    // 접속 상태를 '해제 상태'로 초기화함.
    m_conn_flag = 0;

    // 소켓 핸들을 초기화함.
    mh_socket = INVALID_SOCKET;

    // FD_CONNECT 이벤트 발생 시에 사용할 윈도우 메시지 번호를 기억함.
    m_conn_notify_id = a_conn_notify_id;
}

// BH_ClientSocket 클래스의 객체 소멸자 ~BH_ClientSocket
BH_ClientSocket::~BH_ClientSocket()
{
    // 서버와 통신하기 위해 소켓이 생성되어 있다면 소켓을 제거함.
    if (mh_socket != INVALID_SOCKET)
        closesocket(mh_socket);
}

// 서버에 접속을 시도하는 함수 ConnToServer의 정의
int BH_ClientSocket::ConnToServer(const wchar_t* ap_ip_addr, int a_port_num, HWND ah_notify_wnd)
{
    // 접속을 시도 중이거나 접속된 상태라면 접속을 시도하지 않음.
    if (m_conn_flag != 0)
        return 0;   // 중복 시도 또는 중복 접속 오류를 의미함.

    // 소켓 이벤트로 인한 윈도우 메시지를 받은 윈도우의 핸들을 저장함.
    mh_notify_wnd = ah_notify_wnd;

    /* 서버와 통신할 소켓을 생성함. (TCP)
       SOCK_STREAM이 TCP를 의미함. */
    mh_socket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in srv_addr;
    char temp_ip_addr[16];

    // 유니코드 형식으로 전달된 IP 주소를 ASCII 형식으로 변경함.
    UnicToAsc(temp_ip_addr, (wchar_t*)ap_ip_addr);

    // 서버에 접속하기 위하여 서버의 IP 주소와 포트 번호로 접속 정보를 구성함.
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = inet_addr(temp_ip_addr);
    /* htons 함수는 호스트 바이트 오더를 네트웍 바이트 오더로 변환해 주는 함수이다.
       예를 들어 10400의 Port 번호를 htons 함수를 통해 네트웍 바이트 오더로 변경한다면
       먼저 "10400"이 이진값으로 변경되어 "00101000 10100000"이 될 것이고
       이를 8bit 단위로 나누어 바이트 순서를 바꿔준다면 "10100000 00101000"
       의 값이 된다. 이 값을 계산해보면 "41000"이 된다. */
    srv_addr.sin_port = htons(a_port_num);

    /* 서버에 접속하는 connect 함수가 응답없음 상태에 빠질 수 있기 때문에 비동기를 설정함.
       서버 접속에 대한 결과인 FD_CONNECT 이벤트가 발생하면 ah_notify_wnd에
       해당하는 윈도우로 m_conn_notify_id에 해당하는 윈도우 메시지를 전송함. */
    WSAAsyncSelect(mh_socket, ah_notify_wnd, m_conn_notify_id, FD_CONNECT);

    // 서버에 접속을 시도함.
    connect(mh_socket, (sockaddr*)&srv_addr, sizeof(srv_addr));

    // 접속 상태를 '접속 시도중'으로 변경함.
    m_conn_flag = 1;
    return 1;
}

// 서버 접속에 대한 결과가 메시지로 전달되었을 때 사용하는 함수 ResultOfConn의 정의
int BH_ClientSocket::ResultOfConn(LPARAM lParam)
{
    // 반환값이 1이면 서버에 접속을 성공함, 0이면 서버에 접속을 실패함.

    if (WSAGETSELECTERROR(lParam) == 0)
    {
        // 접속 에러가 없음. 즉, 서버에 성공적으로 접속함.
        m_conn_flag = 2;   // 접속 상태를 '접속'으로 변경함.

        /* 접속된 소켓으로 서버에서 데이터가 수신되거나 연결이 해제 되었을 때
           윈도우 메시지를 받을 수 있도록 비동기를 설정함. */
        WSAAsyncSelect(mh_socket, mh_notify_wnd, m_data_notify_id, FD_READ | FD_CLOSE);

        return 1;   // 접속 성공을 의미함.
    }
    else
    {
        // 접속에 실패함.
        closesocket(mh_socket);       // 서버와 통신하기 위해 만든 소켓을 제거함.

        mh_socket = INVALID_SOCKET;   // 소켓을 초기화함.
        m_conn_flag = 0;              // 접속 상태를 '해제'로 변경함.
    }
    return 0;   // 접속에 실패함을 의미함.
}

/* 서버가 데이터를 전송하거나 연결을 해제했을 때 발생하는 윈도우 메시지에서
   사용하는 함수 ProcessServerEvent의 정의 */
int BH_ClientSocket::ProcessServerEvent(WPARAM wParam, LPARAM lParam)
{
    // 반환값이 0이면 서버가 접속을 해제, 1이면 서버에서 데이터를 수신함. 

    /* 접속이 해제 되었을 때, 추가적인 메시지를 사용하지 않고 이 함수의 반환 값으로
       구별해서 사용할 수 있도록 FD_READ는 1, FD_CLOSE는 0 값을 반환하도록 구현한다. */
    int state = 1;

    if (WSAGETSELECTEVENT(lParam) == FD_READ)
    {
        /* 서버에서 데이터를 전송한 경우
           수신된 데이터를 처리하기 위한 함수를 호출함. */
        ProcessRecvEvent((SOCKET)wParam);
    }
    else
    {
        // FD_CLOSE, 서버가 접속을 해제한 경우 
        state = 0;
        m_conn_flag = 0;              // 접속 상태를 '해제'로 변경함.
        closesocket(mh_socket);       // 서버와 통신하기 위해 생성한 소켓을 제거함.
        mh_socket = INVALID_SOCKET;   // 소켓 핸들을 저장하는 변수를 초기화함. 
    }

    return state;   // 이벤트 종류를 반환함.
}

// 서버와 강제로 접속을 해제할 때 사용하는 함수 DisconnSocket의 정의
void BH_ClientSocket::DisconnSocket(SOCKET ah_socket, int a_err_code)
{
    // 접속 상태를 '해제'로 변경함.
    m_conn_flag = 0;
    LINGER temp_linger = { TRUE, 0 };

    /* 서버에서 데이터를 수신하고 있는 상태라면 강제로 소켓을 제거하지 못하기 때문에
       링거 옵션을 설정하여 데이터를 수신하고 있더라도 소켓을 제거할 수 있도록 함. */

       /* setsockopt 함수는 소켓의 송수신 동작을 다양한 옵션으로 제어할 수 있는 함수이다.

          setsockopt 함수의 기본형

          int setsockopt(SOCKET socket, int level, int optname, const void* optval, int optlen);

          1. socket : 소켓의 번호
          2. level : 옵션의 종류. 보통 SOL_SOCKET 와 IPPROTO_TCP 중 하나를 사용
          3. optname : 설정을 위한 소켓 옵션의 번호
          4. optval : 설정 값이 저장된 주소 값
          5. optlen : optval 버퍼의 크기

          SOL_SOCKET 이란?
          SOL_SOCKET 레벨 옵션은 소켓 코드에서 해석하여 처리하므로 프로토콜에 독립적임.
          그러나 여기에 속한 옵션을 모든 프로토콜에 적용할 수 있는 것은 아니므로 주의해야 함.

          IPPROTO_TCP 이란?
          IPPROTO_TCP 레벨 옵션은 TCP 프로토콜 코드에서 해석하여 처리함.
          따라서 TCP 소켓에 대해서만 적용할 수 있음. */
    setsockopt(mh_socket, SOL_SOCKET, SO_LINGER, (char*)&temp_linger, sizeof(temp_linger));

    // 소켓을 제거함.
    closesocket(mh_socket);

    // 소켓 핸들 값을 저장하는 변수를 초기화 함.
    mh_socket = INVALID_SOCKET;
}

// 서버로 데이터를 전송할 때 사용하는 함수 SendFrameData의 정의
int BH_ClientSocket::SendFrameData(unsigned char a_msg_id, const char* ap_body_data, BSize a_body_size)
{
    return BH_Socket::SendFrameData(mh_socket, a_msg_id, ap_body_data, a_body_size);
}

// FD_READ 이벤트가 발생했을 때 실제 데이터를 처리하는 함수 ProcessRecvData의 정의 
int BH_ClientSocket::ProcessRecvData(SOCKET ah_socket, unsigned char a_msg_id, char* ap_recv_data,
    BSize a_body_size)
{
    if (a_msg_id == 251)
    {
        // 예약 메시지 251번은 서버에 큰 용량의 데이터를 전송하기 위해 사용함.
        char* p_send_data;

        // 현재 전송 위치를 얻음.
        BSize send_size = m_send_man.GetPos(&p_send_data);

        // 전송할 데이터가 더 있다면 예약 메시지 번호인 252를 사용하여 서버에게 데이터를 전송함.
        if (m_send_man.IsProcessing())
        {
            BH_Socket::SendFrameData(mh_socket, 252, p_send_data, send_size);
        }
        else
        {
            /* 지금이 분할된 데이터의 마지막 부분이라면 예약 메시지 번호인 253번을 사용하여
               서버에게 데이터를 전송함. */
            BH_Socket::SendFrameData(mh_socket, 253, p_send_data, send_size);

            // 마지막 데이터를 전송하고 나서 전송에 사용했던 메모리를 삭제함.
            m_send_man.DelData();

            /* 클라이언트 소켓을 사용하는 윈도우에 전송이 완료되었음을 알려줌.
               전송이 완료되었을 때 프로그램에 어떤 표시를 하고 싶다면 해당 윈도우에서
               LM_SEND_COMPLETED 메시지를 체크하면 됨. */
            ::PostMessage(mh_notify_wnd, LM_SEND_COMPLETED, 0, 0);
        }
    }
    else if (a_msg_id == 252)
    {
        /* 252번은 대용량의 데이터를 수신할 때 사용하는 예약된 메시지 번호임.
           수신된 데이터는 수신을 관리하는 객체로 넘겨서 데이터를 합쳐야 함. */
        m_recv_man.AddData(ap_recv_data, a_body_size);

        /* 252번은 아직도 추가로 수신할 데이터가 있다는 뜻이기 때문에 예약 메시지
           251번을 서버에 전송하여 추가 데이터를 요청함. */
        BH_Socket::SendFrameData(mh_socket, 251, NULL, 0);
    }
    else if (a_msg_id == 253)
    {
        /* 253번은 대용량의 데이터를 수신할 때 사용하는 예약된 메시지 번호임.
           수신된 데이터는 수신을 관리하는 객체로 넘겨서 데이터를 합쳐야 함. */
        m_recv_man.AddData(ap_recv_data, a_body_size);

        /* 253번은 데이터 수신이 완료되었다는 메시지이기 때문에 클라이언트 소켓을
           사용하는 윈도우에 완료되었음을 알려줌.

           따라서 윈도우에서 수신이 완료된 데이터를 사용하려면 LM_RECV_COMPLETED
           메시지를 사용하면 됨.

           LM_RECV_COMPLETED 메시지를 수신한 처리기에서 수신할 때 사용한 메모리를
           DelData 함수를 호출해서 제거해야 함. */
        ::PostMessage(mh_notify_wnd, LM_RECV_COMPLETED, 0, 0);
    }

    /* 수신된 데이터를 정상적으로 처리함.
       만약, 수신 데이터를 처리하던 중에 소켓을 제거했으면 0을 반환해야 함.
       0을 반환하면 이 소켓에 대해서 비동기 작업이 중단됨. */
    return 1;
}



