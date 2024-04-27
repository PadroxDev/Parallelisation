#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <WS2tcpip.h>

const char* ADDR_IP = ""; // IP
const int SERVER_PORT = 12345;

class Papagnan {
public:
    HINSTANCE hInstance;
    HWND mainHwnd;
    HWND hiddenHwnd;
    SOCKET clientSocket;

    bool running;

    Papagnan(HINSTANCE hInst) {
        hInstance = hInst;
    }

    ~Papagnan() {

    }

    int InitWindow(int nCmdShow) {
        WNDCLASS mainHwndClass = {};
        mainHwndClass.lpfnWndProc = WindowProc; // WindowProc sera crée plus loin
        mainHwndClass.hInstance = hInstance;
        mainHwndClass.lpszClassName = L"MainWindowClass";

        if (!RegisterClass(&mainHwndClass)) {
            DWORD error = GetLastError();
            if (error != ERROR_CLASS_ALREADY_EXISTS) {
                OutputDebugString(L"RegisterClass failed with error: %d\n" + error);
                return false;
            }
        }

        mainHwnd = CreateWindowEx(
            0,                              // Options de la fenêtre
            L"MainWindowClass",               // Nom de classe de la fenêtre
            L"Chat Client",                 // Titre de la fenêtre
            WS_OVERLAPPEDWINDOW,            // Style de la fenêtre
            CW_USEDEFAULT, CW_USEDEFAULT,   // Position de la fenêtre (par défaut)
            800, 600,                       // Taille de la fenêtre
            NULL,                           // Handle de la fenêtre parent
            NULL,                           // Handle du menu
            hInstance,                      // Instance de l'application
            NULL                            // Données de création supplémentaires
        );
        if (mainHwnd == NULL)
        {
            MessageBox(NULL, L"Failed to create window!", L"Error", MB_OK | MB_ICONERROR);
            return 1;
        }

        SetWindowLongPtr(mainHwnd, GWLP_USERDATA, (LONG_PTR)this);

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

        ShowWindow(mainHwnd, nCmdShow);
    }

    int InitNetwork() {
        WSADATA wsaData; // Data struct pour winsock

        // Check de l'init en version 2.2
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            MessageBox(NULL, L"Initialize Failed!", L"Error", MB_OK | MB_ICONERROR);
            return 0;
        }

        clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // Init
        if (clientSocket == INVALID_SOCKET) {
            MessageBox(NULL, L"Failed to create socket!", L"Error", MB_OK | MB_ICONERROR);
            WSACleanup(); // Nettoyer Winsock
            return 0;
        }

        // Remplissage de la structure sockaddr_in
        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET; // UDP / TCP
        serverAddr.sin_addr.s_addr = inet_addr(ADDR_IP); // IP
        serverAddr.sin_port = htons(SERVER_PORT); // Port

        if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) { // On utiliser connect pour faire le lien
            MessageBox(NULL, L"Failed to connect!", L"Error", MB_OK | MB_ICONERROR);
            closesocket(clientSocket);
            WSACleanup(); // Nettoyer Winsock
            return 1;
        }

        if (WSAAsyncSelect(clientSocket, hiddenHwnd, WM_USER + 1, FD_READ || FD_CLOSE)) {

            OutputDebugString(L"ALED\n");
            return 0;
        }

        running = true;
    }

    void Run() {
        Send("Coucou Papa");
        while (running) {
            MSG msg = {};
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) // Recois en continu les window events
            {
                TranslateMessage(&msg); // Traduit les inputs par exemple
                DispatchMessage(&msg); // Envoie les messages à WindowProc
            }
        }
    }

    void Send(const char* msg) {
        send(clientSocket, msg, strlen(msg), 0);
    }

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        Papagnan* papagnan = (Papagnan*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

        switch (uMsg)
        {
        case WM_DESTROY: // Si on appuie sur la croix
            PostQuitMessage(0); // Quitter l'app
            papagnan->running = false;
            delete papagnan;
            return 0;
        case WM_LBUTTONDOWN: // Si on appuie sur le clic gauche
        {
            int x = LOWORD(lParam); // Coordonnées X du clic
            int y = HIWORD(lParam); // Coordonnées Y du clic

            HDC hdc = GetDC(hwnd); // On récupère le context window

            for (int i = 0; i < 21; i++)
            {
                for (int j = 0; j < 21; j++)
                {
                    SetPixel(hdc, x + i - 10, y + j - 10, RGB(0, 0, 255)); // On set le pixel bleu
                }
            }
            ReleaseDC(hwnd, hdc); // On libère le context window
        }
        return 0;
        }
        // On renvoie au Protocol par défaut les events non gérés ici
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    static LRESULT CALLBACK HiddenWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        Papagnan* papagnan = (Papagnan*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

        switch (uMsg)
        {
        case WM_USER + 1:
            WORD msg = WSAGETSELECTEVENT(lParam);
            if (msg == FD_READ) {
                OutputDebugString(L"READDDDDDDDDDDDDDDDDDDDDDDDDD");
            }
            return 0;
        }
        // On renvoie au Protocol par défaut les events non gérés ici
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    Papagnan* papagnan = new Papagnan(hInstance);
    papagnan->InitWindow(nCmdShow);
    papagnan->InitNetwork();
    papagnan->Run();
}