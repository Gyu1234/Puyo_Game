#include <SFML/Graphics.hpp>
#include <array>
#include <vector>
#include <queue>
#include <random>
#include <ctime>
#include <algorithm>
#include <sstream>
#include <iomanip>

using namespace std;

// ---- 자주 사용하는 색상 상수 정의 ----
const sf::Color GRAY(128, 128, 128);
const sf::Color DARK_GRAY(64, 64, 64);
const sf::Color LIGHT_GRAY(192, 192, 192);
const sf::Color ORANGE(255, 165, 0);
const sf::Color DARK_GREEN(0, 128, 0);
const sf::Color DARK_BLUE(0, 0, 128);
const sf::Color PINK(255, 192, 203);

// ---- 기본 상수 ----
static const int COLS = 6;     // 가로 칸 수 (뿌요뿌요 기본은 6)
static const int ROWS = 12;    // 세로 칸 수 (기본은 12)
static const int CELL = 32;    // 셀 하나의 픽셀 크기
static const int WINDOW_WIDTH = COLS * CELL + 200; // 점수 표시를 위한 추가 공간
static const int WINDOW_HEIGHT = ROWS * CELL;

// 게임 상태
enum GameState { MENU, PLAYING, GAME_OVER };

// 셀의 상태 (빈칸 또는 색상)
enum Color { EMPTY=0, RED, GREEN, BLUE, YELLOW, PURPLE, COLOR_COUNT };

// 2차원 좌표 표현용
struct Vec2 { int x, y; };

// 조작 중인 뿌요쌍 구조체
struct PuyoPair {
    Vec2 pivot;   // 중심 뿌요(회전 중심)의 좌표
    Vec2 sub;     // 서브 뿌요의 상대 좌표 (pivot 기준)
    Color c1, c2; // pivot, sub의 색상
};

// ---- 유틸 함수 ----
bool inBounds(int x, int y){ return x>=0 && x<COLS && y>=0 && y<ROWS; }

// 랜덤 엔진 (매번 초기화되지 않도록 static)
std::mt19937& rng() {
    static std::mt19937 gen(static_cast<unsigned>(time(nullptr)));
    return gen;
}

// 랜덤 색상 뽑기
Color randomColor() {
    std::uniform_int_distribution<int> dist(1, COLOR_COUNT-1);
    return static_cast<Color>(dist(rng()));
}

// ---- 보드 클래스 ----
struct Board {
    array<array<Color, COLS>, ROWS> g{}; // 그리드 (ROWS x COLS)
    int score = 0;  // 점수
    int chain = 0;  // 현재 연쇄 수
    int level = 1;  // 레벨

    Board(){ clear(); }

    // 보드 초기화
    void clear(){
        for(int y=0;y<ROWS;++y) for(int x=0;x<COLS;++x) g[y][x]=EMPTY;
        score=0; chain=0; level=1;
    }

    // 특정 위치가 비어있는지 체크
    bool isEmpty(int x,int y) const {
        return inBounds(x,y) && g[y][x]==EMPTY;
    }

    // 현재 조각이 충돌하는지 판정
    bool collision(const PuyoPair& p) const {
        // pivot
        if(!inBounds(p.pivot.x, p.pivot.y) || g[p.pivot.y][p.pivot.x]!=EMPTY) return true;
        // sub
        int sx = p.pivot.x + p.sub.x;
        int sy = p.pivot.y + p.sub.y;
        if(!inBounds(sx,sy) || g[sy][sx]!=EMPTY) return true;
        return false;
    }

    // 조각을 보드에 고정
    void lock(const PuyoPair& p){
        if(inBounds(p.pivot.x, p.pivot.y)) {
            g[p.pivot.y][p.pivot.x] = p.c1;
        }
        int sx = p.pivot.x + p.sub.x;
        int sy = p.pivot.y + p.sub.y;
        if(inBounds(sx, sy)) {
            g[sy][sx] = p.c2;
        }
    }

    // 중력 처리 (위의 뿌요가 아래로 떨어짐)
    void applyGravity(){
        for(int x=0;x<COLS;++x){
            int write = ROWS-1; // 아래부터 채우기 시작
            for(int y=ROWS-1;y>=0;--y){
                if(g[y][x]!=EMPTY){
                    Color c = g[y][x];
                    g[y][x]=EMPTY;
                    g[write][x]=c;
                    write--;
                }
            }
        }
    }

    // 4개 이상 연결된 뿌요들을 찾아서 제거
    int popGroupsAndScore(int chainIndex){
        vector<vector<bool>> vis(ROWS, vector<bool>(COLS,false));
        int removedTotal = 0;

        // 모든 칸을 탐색
        for(int y=0;y<ROWS;++y){
            for(int x=0;x<COLS;++x){
                if(g[y][x]==EMPTY || vis[y][x]) continue;
                Color c = g[y][x];

                // BFS로 같은 색 연결된 그룹 탐색
                vector<Vec2> group;
                queue<Vec2> q;
                q.push({x,y});
                vis[y][x]=true;
                while(!q.empty()){
                    auto [cx,cy]=q.front(); q.pop();
                    group.push_back({cx,cy});
                    const int dx[4]={1,-1,0,0};
                    const int dy[4]={0,0,1,-1};
                    for(int i=0;i<4;++i){
                        int nx=cx+dx[i], ny=cy+dy[i];
                        if(inBounds(nx,ny) && !vis[ny][nx] && g[ny][nx]==c){
                            vis[ny][nx]=true; q.push({nx,ny});
                        }
                    }
                }

                // 그룹 크기가 4 이상이면 제거
                if((int)group.size()>=4){
                    for(auto &v: group) g[v.y][v.x]=EMPTY;
                    removedTotal += (int)group.size();
                }
            }
        }

        // 점수 계산
        if(removedTotal>0){
            int chainBonus = chainIndex * chainIndex * 50; // 연쇄 보너스
            int popBonus = removedTotal * 10; // 제거 보너스
            score += chainBonus + popBonus;
            
            // 레벨업 (1000점마다)
            level = (score / 1000) + 1;
        }
        return removedTotal;
    }
};

// 회전(90도)
Vec2 rotateCW(const Vec2& v){ return Vec2{ -v.y, v.x }; }     // 시계
Vec2 rotateCCW(const Vec2& v){ return Vec2{ v.y, -v.x }; }    // 반시계

// 벽킥: 충돌 시 좌/우 한 칸 밀어서 회전 가능하게 하는 간단 구현
bool wallKick(Board& b, PuyoPair& p){
    if(!b.collision(p)) return true;
    // 왼쪽
    PuyoPair test = p; test.pivot.x -= 1;
    if(!b.collision(test)){ p = test; return true; }
    // 오른쪽
    test = p; test.pivot.x += 1;
    if(!b.collision(test)){ p = test; return true; }
    return false;
}

// 새 조각 생성 (보드 위쪽에서 등장)
PuyoPair makeSpawnPair(){
    PuyoPair p;
    p.pivot = { COLS/2, 1 };   // 가운데에서 시작 (한 칸 아래로 수정)
    p.sub   = { 0, -1 };       // pivot 위쪽에 붙어 있음
    p.c1 = randomColor();
    p.c2 = randomColor();
    return p;
}

// 이동 가능 판정
bool canMove(Board& b, const PuyoPair& p, int dx, int dy){
    PuyoPair t = p;
    t.pivot.x += dx; t.pivot.y += dy;
    return !b.collision(t);
}

// 색상을 SFML Color로 변환
sf::Color getColor(Color c) {
    switch(c) {
        case RED:    return sf::Color(220,60,60);
        case GREEN:  return sf::Color(60,200,80);
        case BLUE:   return sf::Color(70,120,240);
        case YELLOW: return sf::Color(220,200,60);
        case PURPLE: return sf::Color(160,70,200);
        case EMPTY:  return sf::Color(20,20,20);
        default:     return sf::Color::White;
    }
}

int main(){
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Puyo Puyo Game");
    window.setFramerateLimit(60);

    // 폰트 로드 (시스템 기본 폰트 사용)
    sf::Font font;
    // 폰트가 없어도 게임이 돌아가도록 처리
    bool fontLoaded = font.loadFromFile("arial.ttf"); // Windows 기본 폰트
    if(!fontLoaded) {
        // 다른 경로 시도
        fontLoaded = font.loadFromFile("C:/Windows/Fonts/arial.ttf");
    }

    Board board;
    GameState gameState = MENU;

    // 사각형(블록 하나) 템플릿
    sf::RectangleShape tile(sf::Vector2f((float)CELL-1, (float)CELL-1));

    // 현재 조각 & 게임 상태
    PuyoPair cur = makeSpawnPair();
    PuyoPair nextPair = makeSpawnPair(); // 다음 조각
    bool alive = true;

    // 낙하 속도 제어
    float fallTimer = 0.f;
    float fallInterval = 1.0f; // 기본 낙하 간격(초)

    // 입력 딜레이 방지용
    float moveCooldown = 0.f;
    float rotateCooldown = 0.f;
    const float moveDelay = 0.15f;
    const float rotateDelay = 0.2f;

    sf::Clock clock;

    // 게임 리셋 함수
    auto resetGame = [&](){
        board.clear();
        cur = makeSpawnPair();
        nextPair = makeSpawnPair();
        alive = true;
        fallTimer = 0;
        gameState = PLAYING;
    };

    // 메인 루프
    while(window.isOpen()){
        float dt = clock.restart().asSeconds();

        // 이벤트 처리
        sf::Event e;
        while(window.pollEvent(e)){
            if(e.type == sf::Event::Closed) window.close();
            
            if(e.type == sf::Event::KeyPressed){
                if(gameState == MENU) {
                    if(e.key.code == sf::Keyboard::Space || e.key.code == sf::Keyboard::Return) {
                        resetGame();
                    }
                } else if(gameState == GAME_OVER) {
                    if(e.key.code == sf::Keyboard::R) {
                        resetGame();
                    } else if(e.key.code == sf::Keyboard::Escape) {
                        gameState = MENU;
                    }
                } else if(gameState == PLAYING) {
                    if(e.key.code == sf::Keyboard::Escape) {
                        gameState = MENU;
                    }
                }
            }
        }

        // 게임 로직 (PLAYING 상태에서만 실행)
        if(gameState == PLAYING && alive) {
            // 입력 처리 - 쿨다운 적용
            moveCooldown -= dt;
            rotateCooldown -= dt;

            if(moveCooldown <= 0.f) {
                if(sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
                    if(canMove(board, cur, -1, 0)) {
                        cur.pivot.x -= 1;
                        moveCooldown = moveDelay;
                    }
                } else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
                    if(canMove(board, cur, +1, 0)) {
                        cur.pivot.x += 1;
                        moveCooldown = moveDelay;
                    }
                }
            }

            // 회전 (쿨다운 적용)
            if(rotateCooldown <= 0.f && sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
                PuyoPair t = cur;
                t.sub = rotateCW(t.sub);
                if(board.collision(t)) {
                    if(!wallKick(board, t)) {
                        t = cur;
                        t.sub = rotateCCW(t.sub);
                        wallKick(board, t);
                    }
                }
                cur = t;
                rotateCooldown = rotateDelay;
            }

            // 자동 낙하
            fallTimer += dt;
            float curInterval = fallInterval / (1.0f + board.level * 0.1f); // 레벨에 따른 속도 증가
            
            // 소프트 드랍
            if(sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
                curInterval = 0.05f;
            }

            if(fallTimer >= curInterval) {
                fallTimer = 0.f;
                
                if(canMove(board, cur, 0, +1)) {
                    cur.pivot.y += 1;
                } else {
                    // 보드에 고정
                    board.lock(cur);

                    // 연쇄 처리
                    int chainIndex = 1;
                    bool hadChain = false;
                    while(true) {
                        int removed = board.popGroupsAndScore(chainIndex);
                        if(removed <= 0) break;
                        hadChain = true;
                        board.applyGravity();
                        chainIndex++;
                    }

                    // 다음 조각으로 교체
                    cur = nextPair;
                    nextPair = makeSpawnPair();

                    // 게임오버 체크
                    if(board.collision(cur)) {
                        alive = false;
                        gameState = GAME_OVER;
                    }
                }
            }
        }

        // ----- 렌더링 -----
        window.clear(sf::Color::Black);

        if(gameState == MENU) {
            // 메뉴 화면
            if(fontLoaded) {
                sf::Text title("PUYO PUYO", font, 48);
                title.setFillColor(sf::Color::White);
                title.setPosition(WINDOW_WIDTH/2 - title.getGlobalBounds().width/2, 100);
                window.draw(title);

                sf::Text start("Press SPACE or ENTER to Start", font, 24);
                start.setFillColor(sf::Color::Yellow);
                start.setPosition(WINDOW_WIDTH/2 - start.getGlobalBounds().width/2, 200);
                window.draw(start);

                sf::Text controls1("Controls:", font, 20);
                controls1.setFillColor(sf::Color::Cyan);
                controls1.setPosition(50, 280);
                window.draw(controls1);

                sf::Text controls2("Left/Right Arrow: Move", font, 16);
                controls2.setFillColor(sf::Color::White);
                controls2.setPosition(50, 310);
                window.draw(controls2);

                sf::Text controls3("Up Arrow: Rotate", font, 16);
                controls3.setFillColor(sf::Color::White);
                controls3.setPosition(50, 330);
                window.draw(controls3);

                sf::Text controls4("Down Arrow: Soft Drop", font, 16);
                controls4.setFillColor(sf::Color::White);
                controls4.setPosition(50, 350);
                window.draw(controls4);
            }
        } else if(gameState == GAME_OVER) {
            // 게임오버 화면
            if(fontLoaded) {
                sf::Text gameOver("GAME OVER", font, 48);
                gameOver.setFillColor(sf::Color::Red);
                gameOver.setPosition(WINDOW_WIDTH/2 - gameOver.getGlobalBounds().width/2, 150);
                window.draw(gameOver);

                sf::Text finalScore("Final Score: " + to_string(board.score), font, 24);
                finalScore.setFillColor(sf::Color::White);
                finalScore.setPosition(WINDOW_WIDTH/2 - finalScore.getGlobalBounds().width/2, 220);
                window.draw(finalScore);

                sf::Text restart("Press R to Restart", font, 20);
                restart.setFillColor(sf::Color::Yellow);
                restart.setPosition(WINDOW_WIDTH/2 - restart.getGlobalBounds().width/2, 280);
                window.draw(restart);

                sf::Text menu("Press ESC for Menu", font, 20);
                menu.setFillColor(sf::Color::Yellow);
                menu.setPosition(WINDOW_WIDTH/2 - menu.getGlobalBounds().width/2, 310);
                window.draw(menu);
            }
        } else {
            // 게임 플레이 화면
            // 보드 그리기
            for(int y=0; y<ROWS; ++y) {
                for(int x=0; x<COLS; ++x) {
                    tile.setFillColor(getColor(board.g[y][x]));
                    tile.setPosition((float)x*CELL+0.5f, (float)y*CELL+0.5f);
                    window.draw(tile);
                }
            }

            // 현재 조각 그리기
            if(alive) {
                auto drawPuyo = [&](int x, int y, Color c) {
                    if(inBounds(x, y)) {
                        tile.setFillColor(getColor(c));
                        tile.setPosition((float)x*CELL+0.5f, (float)y*CELL+0.5f);
                        window.draw(tile);
                    }
                };
                
                drawPuyo(cur.pivot.x, cur.pivot.y, cur.c1);
                drawPuyo(cur.pivot.x + cur.sub.x, cur.pivot.y + cur.sub.y, cur.c2);
            }

            // UI 정보 (오른쪽 패널)
            if(fontLoaded) {
                int uiX = COLS * CELL + 10;
                
                sf::Text scoreText("Score: " + to_string(board.score), font, 16);
                scoreText.setFillColor(sf::Color::White);
                scoreText.setPosition(uiX, 50);
                window.draw(scoreText);

                sf::Text levelText("Level: " + to_string(board.level), font, 16);
                levelText.setFillColor(sf::Color::White);
                levelText.setPosition(uiX, 80);
                window.draw(levelText);

                // 다음 조각 미리보기
                sf::Text nextText("Next:", font, 14);
                nextText.setFillColor(sf::Color::Cyan);
                nextText.setPosition(uiX, 120);
                window.draw(nextText);

                // 작은 타일로 다음 조각 그리기
                sf::RectangleShape smallTile(sf::Vector2f(16, 16));
                smallTile.setFillColor(getColor(nextPair.c1));
                smallTile.setPosition(uiX + 10, 150);
                window.draw(smallTile);

                smallTile.setFillColor(getColor(nextPair.c2));
                smallTile.setPosition(uiX + 10, 170);
                window.draw(smallTile);

                sf::Text escText("ESC: Menu", font, 12);
                escText.setFillColor(GRAY); // 정의된 상수 사용
                escText.setPosition(uiX, WINDOW_HEIGHT - 30);
                window.draw(escText);
            }
        }

        window.display();
    }
    
    return 0;
}