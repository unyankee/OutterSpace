#include <engine.h>
#include <iostream>
#include <stdio.h>

int main()
{

    Engine& engine = Engine::instance();
    engine.init();

    MSG  msg;
    bool quitMessageReceived = false;
    while (!quitMessageReceived)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
            {
                quitMessageReceived = true;
                break;
            }
        }

        engine.update();
    }

    return 0;
}
