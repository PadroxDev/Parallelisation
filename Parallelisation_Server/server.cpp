#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <vector>
#include <string>

using namespace std;

const char* ADDR_IP = "192.168.1.33";
const int SERVER_PORT = 12345;

class Papagnan {
public:
    HINSTANCE hInstance;
    HWND hiddenHwnd;
    SOCKET serverSocket;

    vector<SOCKET> connectedClients;

    bool running;

    Papagnan(HINSTANCE hInst) {
        hInstance = hInst;
    }

    ~Papagnan() {

    }

    int InitWindow(int nCmdShow) {
        WNDCLASS hiddenHwndClass = { 0 };
        hiddenHwndClass.lpfnWndProc = HiddenWindowProc;
        hiddenHwndClass.hInstance = GetModuleHandle(NULL);
        hiddenHwndClass.lpszClassName = L"HiddenWindowClass";

        if (!RegisterClass(&hiddenHwndClass)) {
            DWORD error = GetLastError();
            if (error != ERROR_CLASS_ALREADY_EXISTS) {
                OutputDebugString(L"RegisterClass failed with error: %d\n" + error);
                return false;
            }
        }

        hiddenHwnd = CreateWindowEx(
            0,
            L"HiddenWindowClass",
            NULL,
            0, 0, 0, 0, 0,
            HWND_MESSAGE,
            NULL,
            hInstance,
            NULL
        );

        if (hiddenHwnd == NULL) {
            OutputDebugString(L"CreateWindowEx failed with error: %d\n" + GetLastError());
            return false;
        }

        SetWindowLongPtr(hiddenHwnd, GWLP_USERDATA, (LONG_PTR)this);
    }

    int InitNetwork() {
        WSADATA wsaData; // Data struct pour winsock

        // Check de l'init en version 2.2
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            MessageBox(NULL, L"Initialize Failed!", L"Error", MB_OK | MB_ICONERROR);
            return 0;
        }

        serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // Init
        if (serverSocket == INVALID_SOCKET) {
            MessageBox(NULL, L"Failed to create socket!", L"Error", MB_OK | MB_ICONERROR);
            WSACleanup(); // Nettoyer Winsock
            return 0;
        }

        // Remplissage de la structure sockaddr_in
        SOCKADDR_IN serverAddr;
        serverAddr.sin_family = AF_INET; // UDP / TCP
        serverAddr.sin_addr.s_addr = inet_addr(ADDR_IP);
        serverAddr.sin_port = htons(SERVER_PORT); // Port

        if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            MessageBox(NULL, L"Failed to bind server!", L"Error", MB_OK | MB_ICONERROR);
            closesocket(serverSocket);
            WSACleanup();
            return 1;
        }

        if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
            MessageBox(NULL, L"Listen failed!", L"Error", MB_OK | MB_ICONERROR);
            closesocket(serverSocket);
            WSACleanup();
            return 1;
        }

        if (WSAAsyncSelect(serverSocket, hiddenHwnd, WM_USER + 1, FD_ACCEPT)) {
            OutputDebugString(L"WSAAsyncSelect failed with error: " + WSAGetLastError() + '\n');
            return 0;
        }

        running = true;
    }

    void Run() {
        MSG msg = {};
        while (GetMessage(&msg, NULL, 0, 0)) // Recois en continu les window events
        {
            TranslateMessage(&msg); // Traduit les inputs par exemple
            DispatchMessage(&msg); // Envoie les messages à WindowProc
        }
    }

    static LRESULT CALLBACK HiddenWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        Papagnan* papagnan = (Papagnan*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

        switch (uMsg)
        {
        case WM_USER + 1:
            switch (WSAGETSELECTEVENT(lParam)) {
            case FD_ACCEPT:
            {
                SOCKET incomingSocket = accept((SOCKET)wParam, NULL, NULL);
                if (incomingSocket == INVALID_SOCKET) {
                    OutputDebugString(L"An error was encountered while trying to accept a socket !" + WSAGetLastError());
                }
                papagnan->connectedClients.push_back(incomingSocket);
                WSAAsyncSelect(incomingSocket, papagnan->hiddenHwnd, WM_USER + 1, FD_READ | FD_CLOSE);
                OutputDebugString(L"A client was successfully accepted by the server !");
                break;
            }
            case FD_READ:
                char buffer[256];
                int bytesReceived;
                do {
                    bytesReceived = recv((SOCKET)wParam, buffer, sizeof(buffer), 0);
                    if (bytesReceived > 0) {
                        //buffer[bytesReceived] = '\0';
                        string s = buffer;
                        std::wstring stemp = std::wstring(s.begin(), s.begin() + bytesReceived);
                        LPCWSTR sw = stemp.c_str();
                        MessageBox(NULL, sw, L"UWU", 0);
                    }
                } while (bytesReceived > 0);
                break;
            default:
                OutputDebugString(L"Unsopported message received: " + WSAGETSELECTEVENT(lParam));
                break;
            }
        default:
            // On renvoie au Protocol par défaut les events non gérés ici
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
    }
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    Papagnan* papagnan = new Papagnan(hInstance);
    papagnan->InitWindow(nCmdShow);
    papagnan->InitNetwork();
    papagnan->Run();
}