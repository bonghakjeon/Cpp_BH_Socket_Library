/*
#include "BH_Socket.h"

#pragma comment(lib, "WS2_32.lib")

#ifdef _DEBUG
    #pragma comment(lib, "D_BH_Socket.lib")
#else
    #pragma comment(lib, "R_BH_Socket.lib")
#endif
*/

#ifndef _BH_SOCKET_H_
#define _BH_SOCKET_H_

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <WinSock2.h>

#define LM_SEND_COMPLETED      29001
#define LM_RECV_COMPLETED      29002

typedef unsigned short int BSize;
#define HEAD_SIZE          2+sizeof(BSize)

// 전송과 수신에서 공통적으로 작업할 내용을 구현한 클래스
class BH_EchManager
{
protected:
    // 전송 또는 수신을 위해 할당된 메모리의 전체 크기와 현재 작업중인 크기
    int m_tot_size, m_cur_size;

    // 전송 또는 수신을 위해서 할당된 메모리의 시작 주소
    char* mp_data;

public:
    BH_EchManager();   // 객체 생성자
    ~BH_EchManager();  // 객체 소멸자

    /* 전송 또는 수신에 사용할 메모리를 할당한다.
       a_data_size에 할당할 크기를 명시하면 이 함수의 반환 값으로
       할당된 메모리의 주소를 반환한다.(반환된 주소는 이 클래스가 관리하고 있다.) */
    char* MemoryAlloc(int a_data_size);

    // 전송 또는 수신에서 사용되던 메모리를 제거한다.
    void DelData();

    // 할당된 메모리의 크기를 반환하는 GetTotSize 함수
    inline int GetTotSize() { return m_tot_size; }

    // 현재 작업 중인 메모리의 위치를 반환하는 GetCurSize 함수
    inline int GetCurSize() { return m_cur_size; }
};

class BH_SendManager : public BH_EchManager
{
public:
    // 현재 전송할 위치와 크기를 계산하는 GetPos 함수
    int GetPos(char** ap_data, int a_data_size = 2048);

    // 전송이 완료되었는지 체크하는 IsProcessing 함수
    inline char IsProcessing() { return m_tot_size != m_cur_size; }
};

class BH_RecvManager : public BH_EchManager
{
public:
    // 수신된 데이터를 기존에 수신 되어 있는 데이터에 추가한다.
    int AddData(char* ap_data, int a_size);

    // 수신된 데이터를 하나로 합친 메모리의 시작 주소를 얻는 GetData 함수
    inline char* GetData() { return mp_data; }
};

class BH_Socket
{
protected:
    // 프로토콜의 유효성을 체크하기 위한 구분 값을 저장할 변수
    unsigned char m_val_key;

    // 전송과 수신에 사용할 버퍼
    char* mp_send_data, * mp_recv_data;

    // 데이터가 수신되거나 상대편이 접속을 해제했을 때 메시지를 수신할 윈도우 핸들 값
    HWND mh_notify_wnd;

    // 데이터가 수신되거나 상대편이 접속을 해제했을 때 사용할 메시지 ID
    int m_data_notify_id;

public:
    /* 객체 생성시에 프로토콜 구분 값과 데이터 수신 및 연결 해제에 사용할
       메시지 ID를 지정해야 함. */
       // BH_Socket 클래스의 객체 생성자 BH_Socket
    BH_Socket(unsigned char a_val_key, int a_data_notify_id);

    // BH_Socket 클래스의 객체 소멸자 ~BH_Socket
    virtual ~BH_Socket();

    /* 연결된 대상에게 네트워크로 데이터를 전송할 때 사용할 함수 SendFrameData
       이 함수는 전달된 정보를 가지고 하나의 메모리에 약속된 Head 정보를 구성해서 전송함. */
    int SendFrameData(SOCKET ah_socket, unsigned char a_msg_id, const char* ap_body_data, BSize a_body_size);

    // 재시도 수신을 지원하는 함수 ReceiveData
    int ReceiveData(SOCKET ah_socket, BSize a_body_size);

    // 데이터가 수신되었을 때 수신된 데이터를 처리하는 함수 ProcessRecvEvent
    void ProcessRecvEvent(SOCKET ah_socket);

    // 접속된 대상을 끊을 때 사용하는 함수 - 자식 클래스에서 꼭 재정의해서 사용해야 함.
    virtual void DisconnSocket(SOCKET ah_socket, int a_err_code);

    // 수신된 데이터를 처리하는 함수 - 자식 클래스에서 꼭 재정의해서 사용해야 함.
    virtual int ProcessRecvData(SOCKET ah_socket, unsigned char a_msg_id, char* ap_recv_data, BSize a_body_size);

    // ASCII 형식의 문자열을 유니코드로 변환함.
    static void AscToUnic(wchar_t* ap_dest_ip, char* ap_src_ip);

    // 유니코드 형식의 문자열을 ASCII로 변환함.
    static void UnicToAsc(char* ap_dest_ip, wchar_t* ap_src_ip);
};

class BH_UserData
{
protected:
    // 클라이언트와 통신하기 위해 사용할 소켓 핸들
    SOCKET mh_socket;

    // 클라이언트에게 큰 데이터를 전송하기 위해 사용할 객체
    BH_SendManager* mp_send_man;

    // 클라이언트에게 큰 데이터를 수신하기 위해 사용할 객체
    BH_RecvManager* mp_recv_man;

    // 클라이언트의 IP 주소
    wchar_t m_ip_addr[16];

public:
    BH_UserData();            // BH_UserData 클래스의 객체 생성자 BH_UserData
    virtual ~BH_UserData();   // BH_UserData 클래스의 객체 소멸자 ~BH_UserData

    // 소켓 핸들 값을 얻는 함수 GetHandle
    inline SOCKET GetHandle() { return mh_socket; }

    // 소켓 핸들 값을 설정하는 함수 SetHandle
    inline void SetHandle(SOCKET ah_socket) { mh_socket = ah_socket; }

    // 전송용 객체의 주소를 얻는 함수 GetSendMan
    inline BH_SendManager* GetSendMan() { return mp_send_man; }

    // 수신용 객체의 주소를 얻는 함수 GetRecvMan
    inline BH_RecvManager* GetRecvMan() { return mp_recv_man; }

    // 클라이언트의 IP 주소를 얻는 함수 GetIP
    wchar_t* GetIP();

    // 클라이언트의 IP 주소를 설정하는 함수 SetIP
    void SetIP(const wchar_t* ap_ip_addr);

    /* 소켓 정보가 할당되어 있다면 소켓을 닫고 초기화한다.
       a_linger_flag에 1을 명시하면 소켓이 데이터를 수신 중이더라도
       즉시 소켓을 제거한다. */
    void CloseSocket(char a_linger_flag = 0);

    // 다형성 적용 시, 동일한 클래스를 확장할 때 사용하는 함수 CreateObj
    virtual BH_UserData* CreateObj() { return new BH_UserData; }

};

class BH_ServerSocket : public BH_Socket
{
protected:
    // listen 작업에 사용할 소켓 핸들
    SOCKET mh_listen_socket;

    // 새로운 클라이언트가 접속했을 때 발생할 메시지 ID 값
    int m_accept_notify_id;

    // 서버가 관리할 최대 사용자 수
    unsigned short int m_max_user_cnt;

    // 최대 사용자를 저장하기 위해서 사용할 객체들
    BH_UserData** mp_user_list;

public:
    // BH_ServerSocket 클래스의 객체 생성자 BH_ServerSocket
    BH_ServerSocket(unsigned char a_val_key, unsigned short int a_max_user_cnt, BH_UserData* ap_user_data,
        int a_accept_notify_id = 25001, int a_data_notify_id = 25002);

    // BH_ServerSocket 클래스의 객체 소멸자 ~BH_ServerSocket
    virtual ~BH_ServerSocket();

    /* 서버 서비스를 시작하는 함수 StartServer
       반환값이 1이면 소켓 생성 성공, -1이면 소켓 생성 실패,
       -2이면 생성된 소켓을 바인드 하는데 실패를 의미함. */
    int StartServer(const wchar_t* ap_ip_addr, int a_port, HWND ah_notify_wnd);

    // Accept 시에 추가적으로 해야할 작업이 있다면 이 함수를 오버라이딩해서 처리해야함.
    virtual void AddWorkForAccept(BH_UserData* ap_user) { }

    // 최대 사용자 수 초과 시에 추가적으로 해야할 작업이 있다면 이 함수를 오버라이딩해서 처리해야함.
    virtual void ShowLimitErr(const wchar_t* ap_ip_addr) { }

    /* 새로운 클라이언트가 접속할 때 사용할 함수 ProcessToAccpet
       - FD_ACCEPT와 연결된 메시지가 발생했을 때 사용함.
       함수의 반환값이 1이면 accept 성공, -1이면 accept 실패,
       -2이면 접속 가능한 최대 사용자 수 초과를 의미함. */
    int ProcessToAccept(WPARAM wParam, LPARAM lParam);

    /* 클라이언트가 접속 해제 시에 추가적으로 해야할 작업이 있다면
       이 함수를 오버라이딩해서 처리해야함.
       a_err_code : 0이면 정상 종료, -1이면 키값이 유효하지 않아서 종료,
                    -2이면 바디 정보 수신 중에 오류가 발생함을 의미함. */
    virtual void AddWorkForCloseUser(BH_UserData* ap_user, int a_err_code);

    /* 새로운 데이터가 수신(FD_READ)되거나 클라이언트가 연결이 해제(FD_CLOSE)될 때
       발생하는 메시지에서 서버 소켓과 관련된 작업을 처리해주는 함수 ProcessClientEvent */
    void ProcessClientEvent(WPARAM wParam, LPARAM lParam);

    /* 소켓 핸들을 사용하여 어떤 사용자인지 찾는다.
       찾으면 사용자의 위치를 반환하는 함수 FindUserIndex */
    inline int FindUserIndex(SOCKET ah_socket)
    {
        for (int i = 0; i < m_max_user_cnt; i++)
        {
            if (mp_user_list[i]->GetHandle() == ah_socket)
                return i;
        }
        return -1;
    }

    /* 소켓 핸들을 사용하여 어떤 사용자인지 찾는다.
       찾으면 사용자 정보를 관리하는 객체의 주소를 반환하는 함수 FindUserData */
    inline BH_UserData* FindUserData(SOCKET ah_socket)
    {
        for (int i = 0; i < m_max_user_cnt; i++)
        {
            if (mp_user_list[i]->GetHandle() == ah_socket)
                return mp_user_list[i];
        }
        return NULL;
    }

    // 전체 사용자에 대한 정보를 얻을 때 사용하는 함수 GetUserList
    inline BH_UserData** GetUserList() { return mp_user_list; }

    // 색인(a_index)을 사용하여 특정 사용자에 대한 정보를 얻을 때 사용하는 함수 GetUserData
    inline BH_UserData* GetUserData(int a_index) { return mp_user_list[a_index]; }

    // 최대 사용자 수를 얻을 때 사용하는 함수 GetMaxUserCnt
    unsigned short int GetMaxUserCnt() { return m_max_user_cnt; }

    // ah_socket 핸들 값을 사용하는 소켓 사용자를 강제로 종료 시키는 함수 DisconnSocket
    virtual void DisconnSocket(SOCKET ah_socket, int a_err_code);

    /* FD_READ 이벤트가 발생했을 때 실제 데이터를 처리하는 함수 ProcessRecvData
       (서버 소켓을 상속 받은 클래스에서 반드시 재정의 하여 자신이 정의한 메시지를 처리해야 한다.) */
    virtual int ProcessRecvData(SOCKET ah_socket, unsigned char a_msg_id, char* ap_recv_data,
        BSize a_body_size);
};

class BH_ClientSocket : public BH_Socket
{
protected:
    // 서버와 통신하기 위해 사용할 소켓 핸들
    SOCKET mh_socket;

    // 0 : 접속 안됨, 1 : 접속 시도 중, 2 : 접속 중
    char m_conn_flag;

    // 서버에 접속을 시도한 결과를 알려줄 윈도우 메시지를 저장함.
    int m_conn_notify_id;

    // 서버에 큰 데이터를 전송하기 위해 사용할 객체
    BH_SendManager m_send_man;

    // 서버에 큰 데이터를 수신하기 위해 사용할 객체
    BH_RecvManager m_recv_man;

public:
    // BH_ClientSocket 클래스의 객체 생성자 BH_ClientSocket
    BH_ClientSocket(unsigned char a_val_key, int a_conn_notify_id = 26001,
        int a_data_notify_id = 26002);

    // BH_ClientSocket 클래스의 객체 소멸자 ~BH_ClientSocket
    virtual ~BH_ClientSocket();

    // 전송용 객체의 주소를 얻는다.
    inline BH_SendManager* GetSendMan() { return &m_send_man; }

    // 수신용 객체의 주소를 얻는다.
    inline BH_RecvManager* GetRecvMan() { return &m_recv_man; }

    // 서버에 접속을 시도하는 함수 ConnToServer
    int ConnToServer(const wchar_t* ap_ip_addr, int a_port_num, HWND ah_notify_wnd);

    /* 서버 접속에 대한 결과가 메시지로 전달되었을 때 이 함수를 사용하면 된다.
       반환값이 1이면 서버에 접속을 성공함, 0이면 서버에 접속을 실패함. */
    int ResultOfConn(LPARAM lParam);

    /* 서버가 데이터를 전송하거나 연결을 해제했을 때 발생하는 윈도우 메시지에서
       사용하는 함수 ProcessServerEvent
       반환값이 0이면 서버가 접속을 해제, 1이면 서버에서 데이터를 수신함. */
    int ProcessServerEvent(WPARAM wParam, LPARAM lParam);

    // 서버와 강제로 접속을 해제할 때 사용하는 함수 DisconnSocket
    virtual void DisconnSocket(SOCKET ah_socket, int a_err_code);

    /* FD_READ 이벤트가 발생했을 때 실제 데이터를 처리하는 함수 ProcessRecvData
       (서버 소켓을 상속 받은 클래스에서 반드시 재정의하여 자신이 정의한 메시지를 처리해야 한다.) */
    virtual int ProcessRecvData(SOCKET ah_socket, unsigned char a_msg_id, char* ap_recv_data,
        BSize a_body_size);

    /* 서버로 데이터를 전송할 때 사용하는 함수 SendFrameData
       메시지 ID와 전송할 데이터의 주소, 데이터의 크기를 넘겨주면
       약속된 프로토콜 형식으로 구성해서 전송합니다. */
    int SendFrameData(unsigned char a_msg_id, const char* ap_body_data, BSize a_body_size);

    /* 서버와의 접속 상태를 알고 싶을 때 사용하는 함수 IsConnected
       0 : 해제 상태, 1 : 접속 상태를 의미함.
       내부적으로는 상태를 세 가지로 관리하지만 외부에 알려줄 때는
       두 가지 상태로 알려준다. '접속 시도 중' 상태는 해제로 간주한다. */
    inline int IsConnected() { return m_conn_flag == 2; }

    // 서버와 통신하기 위해 생성한 소켓의 핸들 값을 알고 싶을 때 사용한다. 
    inline SOCKET GetHandle() { return mh_socket; }
};

#endif

