#include <SFML/Graphics.hpp>
#include <array>
#include <vector>
#include <queue>
#include <random>
#include <ctime>
#include <algorithm>

using namespace std;

// ---- 기본 상수 ----
static const int COLS = 6;     // 가로 칸 수 (뿌요뿌요 기본은 6)
static const int ROWS = 12;    // 세로 칸 수 (기본은 12)
static const int CELL = 32;    // 셀 하나의 픽셀 크기

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

    Board(){ clear(); }

    // 보드 초기화
    void clear(){
        for(int y=0;y<ROWS;++y) for(int x=0;x<COLS;++x) g[y][x]=EMPTY;
        score=0; chain=0;
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
        g[p.pivot.y][p.pivot.x] = p.c1;
        g[p.pivot.y + p.sub.y][p.pivot.x + p.sub.x] = p.c2;
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
    // chainIndex는 몇 연쇄째인지
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
            // 간단한 체인 보너스: 40 * 연쇄수 + 제거한 수치당 10점
            score += 40 * chainIndex + removedTotal*10;
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
    p.pivot = { COLS/2, 0 };   // 가운데 위에서 시작
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

int main(){
    sf::RenderWindow window(sf::VideoMode(COLS*CELL, ROWS*CELL), "Puyo Puyo (Minimal)");
    window.setFramerateLimit(60);

    Board board;

    // 사각형(블록 하나) 템플릿
    sf::RectangleShape tile(sf::Vector2f((float)CELL-1, (float)CELL-1));

    // 현재 조각 & 게임 상태
    PuyoPair cur = makeSpawnPair();
    bool alive = true;

    // 낙하 속도 제어
    float fallTimer = 0.f;
    float fallInterval = 0.5f; // 기본 낙하 간격(초)

    // 좌우 이동 입력 딜레이 방지용
    float moveCooldown=0.f, moveDelay=0.1f;

    sf::Clock clock;

    // 게임 리셋 함수
    auto resetGame = [&](){
        board.clear();
        cur = makeSpawnPair();
        alive = true;
        fallTimer=0; board.chain=0; board.score=0;
    };

    // 메인 루프
    while(window.isOpen()){
        float dt = clock.restart().asSeconds();

        // 이벤트 처리
        sf::Event e;
        while(window.pollEvent(e)){
            if(e.type == sf::Event::Closed) window.close();
            if(e.type == sf::Event::KeyPressed){
                if(e.key.code == sf::Keyboard::R) resetGame();
            }
        }

        // 입력 처리
        moveCooldown -= dt;
        if(alive){
            if(moveCooldown<=0.f){
                if(sf::Keyboard::isKeyPressed(sf::Keyboard::Left)){
                    if(canMove(board, cur, -1, 0)){ cur.pivot.x -= 1; moveCooldown=moveDelay; }
                }else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Right)){
                    if(canMove(board, cur, +1, 0)){ cur.pivot.x += 1; moveCooldown=moveDelay; }
                }
            }

            // 소프트 드랍
            if(sf::Keyboard::isKeyPressed(sf::Keyboard::Down)){
                if(canMove(board, cur, 0, +1)){ cur.pivot.y += 1; fallTimer=0; }
            }

            // 회전
            if(sf::Keyboard::isKeyPressed(sf::Keyboard::Up)){
                // 시계 회전
                PuyoPair t = cur; t.sub = rotateCW(t.sub);
                if(board.collision(t)){
                    // 월킥 실패 시 반시계도 시도
                    if(!wallKick(board, t)){
                        t = cur; t.sub = rotateCCW(t.sub);
                        if(board.collision(t)) wallKick(board, t);
                    }
                }
                cur = t;
            }
        }

        // 자동 낙하
        if(alive){
            fallTimer += dt;
            float curInterval = fallInterval;
            if(sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) curInterval = 0.05f; // 빠른 하강

            if(fallTimer >= curInterval){
                fallTimer = 0.f;
                if(canMove(board, cur, 0, +1)){
                    // 한 칸 아래로
                    cur.pivot.y += 1;
                }else{
                    // 더 내려갈 수 없으면 보드에 고정
                    int subY = cur.pivot.y + cur.sub.y;
                    if(cur.pivot.y < 0 || subY < 0){
                        // 보드 밖이면 게임오버
                        alive=false;
                    }else{
                        board.lock(cur);

                        // 연쇄 처리
                        int chainIndex = 1;
                        while(true){
                            int removed = board.popGroupsAndScore(chainIndex);
                            if(removed<=0) break;     // 더 이상 제거할 그룹 없음
                            board.applyGravity();     // 위에 있는 뿌요 내려주기
                            chainIndex++;
                        }

                        // 새 조각 소환
                        cur = makeSpawnPair();
                        if(board.collision(cur)){
                            alive=false; // 스폰하자마자 막히면 게임오버
                        }
                    }
                }
            }
        }

        // 난이도 상승: 시간이 지날수록 낙하속도 빨라짐
        fallInterval = std::max(0.12f, fallInterval - dt*0.0008f);

        // ----- 렌더링 -----
        window.clear();

        // 보드 그리기
        for(int y=0;y<ROWS;++y){
            for(int x=0;x<COLS;++x){
                if(board.g[y][x]==EMPTY){
                    tile.setFillColor(sf::Color(20,20,20)); // 빈칸은 어둡게
                }else{
                    // 색상에 맞게 출력
                    switch(board.g[y][x]){
                        case RED:    tile.setFillColor(sf::Color(220,60,60)); break;
                        case GREEN:  tile.setFillColor(sf::Color(60,200,80)); break;
                        case BLUE:   tile.setFillColor(sf::Color(70,120,240)); break;
                        case YELLOW: tile.setFillColor(sf::Color(220,200,60)); break;
                        case PURPLE: tile.setFillColor(sf::Color(160,70,200)); break;
                        default: tile.setFillColor(sf::Color::White); break;
                    }
                }
                tile.setPosition((float)x*CELL+0.5f, (float)y*CELL+0.5f);
                window.draw(tile);
            }
        }

        // 현재 조각 그리기
        if(alive){
            auto drawCell = [&](int x,int y, Color c){
                if(!inBounds(x,y)) return;
                switch(c){
                    case RED:    tile.setFillColor(sf::Color(220,60,60)); break;
                    case GREEN:  tile.setFillColor(sf::Color(60,200,80)); break;
                    case BLUE:   tile.setFillColor(sf::Color(70,120,240)); break;
                    case YELLOW: tile.setFillColor(sf::Color(220,200,60)); break;
                    case PURPLE: tile.setFillColor(sf::Color(160,70,200)); break;
                    default: tile.setFillColor(sf::Color(180,180,180)); break;
                }
                tile.setPosition((float)x*CELL+0.5f, (float)y*CELL+0.5f);
                window.draw(tile);
            };
            drawCell(cur.pivot.x, cur.pivot.y, cur.c1);
            drawCell(cur.pivot.x + cur.sub.x, cur.pivot.y + cur.sub.y, cur.c2);
        }

        window.display();
    }
    return 0;
}
