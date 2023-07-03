#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>

#include <Windows.h>

int screenW = 120;
int screenH = 40;

float playerX = 6.0f; // X position
float playerY = 6.0f; // Y position
float playerA = 0.0f; // Player turn angle

float playerFOV = 3.1415926 / 4;
float drawDistance = 16.0f;

// Map details
int mapW = 16;
int mapH = 16;

int main()
{
    // Creating the screen buffer
    wchar_t* screen = new wchar_t[ screenW * screenH ];
    HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    SetConsoleActiveScreenBuffer(hConsole);
    DWORD dwBytesWritten = 0;

    // generate map
    std::wstring map;

    // Filling the map
    map += L"################";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#........#######";
    map += L"#..............#";
    map += L"#........#######";
    map += L"#........#.....#";
    map += L"#........#.....#";
    map += L"#........#.....#";
    map += L"#........#..####";
    map += L"#..............#";
    map += L"#......###.....#";
    map += L"#......###.....#";
    map += L"#......###.....#";
    map += L"#..............#";
    map += L"################";

    
    // Clock logic (implementing time.deltaTime() replication)
    auto ts1 = std::chrono::system_clock::now(); // Time Stamp 1
    auto ts2 = std::chrono::system_clock::now(); // Time Stamp 2

    // Game Update/Run loop
    while (true)
    {
        // Clock logic continued:
        ts2 = std::chrono::system_clock::now();
        std::chrono::duration<float> nonStdElapsedTime = ts2 - ts1; // Non Standard Elapsed Time
        ts1 = ts2;
        float elapsedTime = nonStdElapsedTime.count(); // Usable like time.deltaTime()

        // Controls
        if (GetAsyncKeyState((unsigned short)'A') & 0x8000)
            playerA -= (1.0f) * elapsedTime;
        if (GetAsyncKeyState((unsigned short)'D') & 0x8000)
            playerA += (1.0f) * elapsedTime;
        if (GetAsyncKeyState((unsigned short)'W') & 0x8000) {
            playerX += sinf(playerA) * 5.0f * elapsedTime;
            playerY += cosf(playerA) * 5.0f * elapsedTime;

            // Collision detection moving forward
            if (map[(int)playerY * mapW + (int)playerX] == '#')
            {
                playerX -= sinf(playerA) * 5.0f * elapsedTime;
                playerY -= cosf(playerA) * 5.0f * elapsedTime;
            }
        }
        if (GetAsyncKeyState((unsigned short)'S') & 0x8000) {
            playerX -= sinf(playerA) * 5.0f * elapsedTime;
            playerY -= cosf(playerA) * 5.0f * elapsedTime;
            
            // Collision detection moving backwards
            if (map[(int)playerY * mapW + (int)playerX] == '#')
            {
                playerX += sinf(playerA) * 5.0f * elapsedTime;
                playerY += cosf(playerA) * 5.0f * elapsedTime;
            }
        }

        // Left and right strafing not implemented yet, cause some trigonometry issues.
        // Will look at it later
        /*if (GetAsyncKeyState((unsigned short)'A') & 0x8000) {
            playerY -= sinf(playerA) * 5.0f * elapsedTime;
            playerX -= cosf(playerA) * 5.0f * elapsedTime;

            if (map[(int)playerY * mapW + (int)playerX] == '#')
            {
                playerY += sinf(playerA) * 5.0f * elapsedTime;
                playerX += cosf(playerA) * 5.0f * elapsedTime;
            }
        }
        if (GetAsyncKeyState((unsigned short)'D') & 0x8000) {
            playerY += sinf(playerA) * 5.0f * elapsedTime;
            playerX += cosf(playerA) * 5.0f * elapsedTime;

            if (map[(int)playerY * mapW + (int)playerX] == '#')
            {
                playerY -= sinf(playerA) * 5.0f * elapsedTime;
                playerX -= cosf(playerA) * 5.0f * elapsedTime;
            }
        }*/

        for (int x = 0; x < screenW; x++)
        {
            // Calculate where the player is looking
            float playerRayAngle = (playerA - playerFOV / 2) + ((float)x / (float)screenW) * playerFOV;
            
            // Finding a wall to render
            float distanceToWall = 0.0f;
            bool hitWall = false;
            bool boundary = false;

            // Unit vectors for player's ray
            float eyeX = sinf(playerRayAngle);
            float eyeY = cosf(playerRayAngle);

            while (!hitWall && distanceToWall < drawDistance)
            {
                distanceToWall += 0.1f;
                int testX = (int)(playerX + eyeX * distanceToWall);
                int testY = (int)(playerY + eyeY * distanceToWall);
                
                // Test for out of bounds exception
                if (testX < 0 || testY < 0 || testX >= mapW || testY >= mapH)
                {
                    hitWall = true;
                    distanceToWall = drawDistance;
                }
                else
                {
                    // Test for wall detected
                    if (map[testY * mapW + testX] == '#')
                    {
                        hitWall = true;

                        std::vector<std::pair<float, float>> p; // distance to corner, angle between corners

                        // Wall corner pairs detection
                        for (int tx = 0; tx < 2; tx++)
                        {
                            for (int ty = 0; ty < 2; ty++) {
                                float vy = (float)testY + ty - playerY;
                                float vx = (float)testX + tx - playerX;
                                float d = sqrt(vx * vx + vy * vy);
                                float dot = (eyeX * vx / d) + (eyeY * vy / d);
                                p.push_back(std::make_pair(d, dot));
                            }
                        }

                        // sorting the pairs
                        std::sort(p.begin(), p.end(), [](const std::pair<float, float>& left, const std::pair<float, float>& right) { return left.first < right.first; });
                        float bound = 0.01f;
                        if (acos(p.at(0).second) < bound) boundary = true;
                        if (acos(p.at(1).second) < bound) boundary = true;
                    }
                }
            }

            // Calculate distance to ceiling and floor
            int ceiling = (float)(screenH / 2.0) - screenH / ((float)distanceToWall);
            int floor = screenH - ceiling;

            // Depth of field render logic for walls
            short wallShade = ' ';
            if (distanceToWall <= drawDistance / 4.0f)      wallShade = '#';//0x2588;
            else if (distanceToWall < drawDistance / 3.0f)  wallShade = '%';//0x2593;
            else if (distanceToWall < drawDistance / 2.0f)  wallShade = '+';//0x2592;
            else if (distanceToWall < drawDistance)         wallShade = '=';//0x2591;
            else                                            wallShade = ' ';

            if (boundary) wallShade = ' ';

            for (int y = 0; y < screenH; y++)
            {
                if (y <= ceiling) {
                    screen[y * screenW + x] = ' ';
                }
                else if(y > ceiling && y <= floor)
                {
                    screen[y * screenW + x] = wallShade;
                }
                else
                {
                    // Depth of field render logic for floors
                    short floorShade = ' ';
                    float floorDepth = 1.0f - (((float)y- screenH / 2.0f) / ((float)screenH / 2.0f));
                    if (floorDepth < 0.25f)             floorShade = ';';//0x2588;
                    else if (floorDepth < 0.5f)         floorShade = ',';//0x2593;
                    else if (floorDepth < 0.75f)        floorShade = '-';//0x2592;
                    else if (floorDepth < 0.9f)         floorShade = '.';//0x2591;
                    else                                floorShade = ' ';
                    screen[y * screenW + x] = floorShade;
                }
            }
        }

        // Set the screen characters to 0, also can be referred to as blanking the screen
        // Sorry I have no fucking idea of what am I doing
        
        // Display stats
        swprintf_s(screen, 40, L"X=%3.2f, Y=%3.2f, A=%3.2f, FPS=%3.2f ", playerX, playerY, playerA, 1.0f / elapsedTime);
        
        // Display Mini-map
        for (int nx = 0; nx < mapW; nx++)
        {
            for (int ny = 0; ny < mapW; ny++)
            {
                screen[(ny + 1) * screenW + nx + 1] = map[ny * mapW + nx];
            }
        }

        // Player Position
        screen[((int)playerY + 1) * screenW + (int)playerX] = 'P';

        // Update frame
        screen[screenW * screenH - 1] = '\0';
        WriteConsoleOutputCharacter(hConsole, screen, screenW * screenH, { 0, 0 }, &dwBytesWritten);
    }

    return 0;
}
