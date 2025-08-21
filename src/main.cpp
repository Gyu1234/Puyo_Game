#include <SFML/Graphics.hpp>
#include <array>
#include <vector>
#include <queue>
#include <random>
#include <ctime>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>

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

// 파티클 구조체 (이펙트용)
struct Particle {
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::Color color;
    float life;
    float maxLife;
    float size;
    
    Particle(sf::Vector2f pos, sf::Vector2f vel, sf::Color col, float lifeTime, float particleSize) 
        : position(pos), velocity(vel), color(col), life(lifeTime), maxLife(lifeTime), size(particleSize) {}
        
    bool update(float dt) {
        position += velocity * dt;
        life -= dt;
        
        // 알파값 감소 (페이드 아웃)
        float alpha = (life / maxLife) * 255.0f;
        color.a = static_cast<sf::Uint8>(std::max(0.0f, alpha));
        
        // 중력 효과
        velocity.y += 50.0f * dt;
        
        // 크기 감소
        size *= 0.995f;
        
        return life > 0;
    }
};

// 점수 표시 이펙트
struct ScoreEffect {
    sf::Vector2f position;
    sf::Vector2f velocity;
    int score;
    float life;
    float maxLife;
    sf::Color color;
    
    ScoreEffect(sf::Vector2f pos, int points, sf::Color col) 
        : position(pos), velocity(0, -50), score(points), life(2.0f), maxLife(2.0f), color(col) {}
        
    bool update(float dt) {
        position += velocity * dt;
        life -= dt;
        velocity.y -= 30.0f * dt; // 위로 올라가면서 감속
        
        // 알파값 감소
        float alpha = (life / maxLife) * 255.0f;
        color.a = static_cast<sf::Uint8>(std::max(0.0f, alpha));
        
        return life > 0;
    }
};

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

// 랜덤 float 생성
float randomFloat(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng());
}

// ---- 보드 클래스 ----
struct Board {
    array<array<Color, COLS>, ROWS> g{}; // 그리드 (ROWS x COLS)
    int score = 0;  // 점수
    int chain = 0;  // 현재 연쇄 수
    int level = 1;  // 레벨
    
    // 이펙트 관련
    vector<Particle> particles;
    vector<ScoreEffect> scoreEffects;
    float screenShake = 0.0f;
    float levelUpEffect = 0.0f;

    Board(){ clear(); }

    // 보드 초기화
    void clear(){
        for(int y=0;y<ROWS;++y) for(int x=0;x<COLS;++x) g[y][x]=EMPTY;
        score=0; chain=0; level=1;
        particles.clear();
        scoreEffects.clear();
        screenShake = 0.0f;
        levelUpEffect = 0.0f;
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
    
    // 파티클 생성 함수
    void createExplosionEffect(int x, int y, Color color) {
        sf::Vector2f center(x * CELL + CELL/2, y * CELL + CELL/2);
        sf::Color particleColor = getColorSFML(color);
        
        // 폭발 파티클 생성
        for(int i = 0; i < 15; i++) {
            float angle = randomFloat(0, 2 * M_PI);
            float speed = randomFloat(50, 150);
            sf::Vector2f velocity(cos(angle) * speed, sin(angle) * speed);
            
            particles.emplace_back(center, velocity, particleColor, 
                                 randomFloat(0.5f, 1.5f), randomFloat(3, 8));
        }
        
        // 화면 흔들림 효과
        screenShake = std::max(screenShake, 0.3f);
    }
    
    // 점수 이펙트 생성
    void createScoreEffect(int x, int y, int points, int chainIndex) {
        sf::Vector2f position(x * CELL + CELL/2, y * CELL + CELL/2);
        sf::Color color = sf::Color::White;
        
        // 연쇄에 따른 색상 변화
        if(chainIndex >= 3) color = sf::Color::Red;
        else if(chainIndex >= 2) color = sf::Color::Yellow;
        else if(points > 100) color = sf::Color::Cyan;
        
        scoreEffects.emplace_back(position, points, color);
    }

    // 4개 이상 연결된 뿌요들을 찾아서 제거
    int popGroupsAndScore(int chainIndex){
        vector<vector<bool>> vis(ROWS, vector<bool>(COLS,false));
        int removedTotal = 0;
        int groupCount = 0;
        vector<Vec2> removedPositions; // 제거된 위치들 저장

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
                    for(auto &v: group) {
                        g[v.y][v.x]=EMPTY;
                        removedPositions.push_back(v);
                        // 폭발 이펙트 생성
                        createExplosionEffect(v.x, v.y, c);
                    }
                    removedTotal += (int)group.size();
                    groupCount++;
                }
            }
        }

        // 점수 계산 및 이펙트
        if(removedTotal>0){
            int baseScore = removedTotal * 10;
            int chainBonus = chainIndex * chainIndex * 50;
            int groupBonus = groupCount > 1 ? (groupCount - 1) * 100 : 0;
            int massBonus = removedTotal >= 8 ? (removedTotal - 7) * 20 : 0;
            
            int totalPoints = baseScore + chainBonus + groupBonus + massBonus;
            score += totalPoints;
            
            // 점수 이펙트 생성 (대표 위치에)
            if(!removedPositions.empty()) {
                Vec2 center = removedPositions[removedPositions.size()/2];
                createScoreEffect(center.x, center.y, totalPoints, chainIndex);
            }
            
            // 레벨업 확인
            int newLevel = std::min(20, (score / 500) + 1);
            if(newLevel > level) {
                level = newLevel;
                score += level * 50;
                levelUpEffect = 2.0f; // 레벨업 이펙트 시간
                
                // 레벨업 파티클 효과
                for(int i = 0; i < 50; i++) {
                    float angle = randomFloat(0, 2 * M_PI);
                    float speed = randomFloat(100, 200);
                    sf::Vector2f pos(COLS * CELL / 2, ROWS * CELL / 2);
                    sf::Vector2f vel(cos(angle) * speed, sin(angle) * speed);
                    particles.emplace_back(pos, vel, sf::Color::Yellow, 2.0f, 5);
                }
            }
        }
        return removedTotal;
    }
    
    // 현재 레벨에 따른 낙하 속도 계산
    float getFallSpeed() const {
        float baseSpeed = 1.0f;
        float speedMultiplier = 1.0f - (level - 1) * 0.04f;
        return std::max(0.1f, baseSpeed * speedMultiplier);
    }
    
    // 이펙트 업데이트
    void updateEffects(float dt) {
        // 파티클 업데이트
        particles.erase(
            std::remove_if(particles.begin(), particles.end(),
                [dt](Particle& p) { return !p.update(dt); }),
            particles.end()
        );
        
        // 점수 이펙트 업데이트
        scoreEffects.erase(
            std::remove_if(scoreEffects.begin(), scoreEffects.end(),
                [dt](ScoreEffect& e) { return !e.update(dt); }),
            scoreEffects.end()
        );
        
        // 화면 흔들림 감소
        screenShake = std::max(0.0f, screenShake - dt * 2.0f);
        
        // 레벨업 이펙트 감소
        levelUpEffect = std::max(0.0f, levelUpEffect - dt);
    }
    
    // 화면 흔들림 오프셋 계산
    sf::Vector2f getShakeOffset() const {
        if(screenShake <= 0) return sf::Vector2f(0, 0);
        
        float intensity = screenShake * 10.0f;
        return sf::Vector2f(
            randomFloat(-intensity, intensity),
            randomFloat(-intensity, intensity)
        );
    }

    // SFML Color 변환
    sf::Color getColorSFML(Color c) const {
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
};

// 회전(90도)
Vec2 rotateCW(const Vec2& v){ return Vec2{ -v.y, v.x }; }
Vec2 rotateCCW(const Vec2& v){ return Vec2{ v.y, -v.x }; }

// 벽킥
bool wallKick(Board& b, PuyoPair& p){
    if(!b.collision(p)) return true;
    PuyoPair test = p; test.pivot.x -= 1;
    if(!b.collision(test)){ p = test; return true; }
    test = p; test.pivot.x += 1;
    if(!b.collision(test)){ p = test; return true; }
    return false;
}

// 새 조각 생성
PuyoPair makeSpawnPair(){
    PuyoPair p;
    p.pivot = { COLS/2, 1 };
    p.sub   = { 0, -1 };
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
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Puyo Puyo Game - Enhanced");
    window.setFramerateLimit(60);

    // 폰트 로드
    sf::Font font;
    bool fontLoaded = font.loadFromFile("arial.ttf");
    if(!fontLoaded) {
        fontLoaded = font.loadFromFile("C:/Windows/Fonts/arial.ttf");
    }

    Board board;
    GameState gameState = MENU;

    // 사각형과 원형 셰이프
    sf::RectangleShape tile(sf::Vector2f((float)CELL-1, (float)CELL-1));
    sf::CircleShape particleShape(3);

    // 현재 조각 & 게임 상태
    PuyoPair cur = makeSpawnPair();
    PuyoPair nextPair = makeSpawnPair();
    bool alive = true;

    // 타이밍 제어
    float fallTimer = 0.f;
    float moveCooldown = 0.f, rotateCooldown = 0.f;
    const float moveDelay = 0.15f, rotateDelay = 0.2f;

    sf::Clock clock;

    // 배경 효과를 위한 시간
    float backgroundTime = 0.0f;

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
        backgroundTime += dt;

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

        // 게임 로직
        if(gameState == PLAYING && alive) {
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

            fallTimer += dt;
            float curInterval = board.getFallSpeed();
            
            if(sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
                curInterval = 0.05f;
            }

            if(fallTimer >= curInterval) {
                fallTimer = 0.f;
                
                if(canMove(board, cur, 0, +1)) {
                    cur.pivot.y += 1;
                } else {
                    board.lock(cur);

                    int chainIndex = 1;
                    while(true) {
                        int removed = board.popGroupsAndScore(chainIndex);
                        if(removed <= 0) break;
                        board.applyGravity();
                        chainIndex++;
                    }

                    cur = nextPair;
                    nextPair = makeSpawnPair();

                    if(board.collision(cur)) {
                        alive = false;
                        gameState = GAME_OVER;
                    }
                }
            }
        }

        // 이펙트 업데이트
        board.updateEffects(dt);

        // ----- 렌더링 -----
        window.clear(sf::Color::Black);

        // 화면 흔들림 적용
        sf::Vector2f shakeOffset = board.getShakeOffset();

        if(gameState == MENU) {
            // 배경 애니메이션
            for(int i = 0; i < 20; i++) {
                float x = sin(backgroundTime + i) * 50 + WINDOW_WIDTH/2;
                float y = cos(backgroundTime * 0.7f + i * 0.5f) * 30 + 100 + i * 15;
                sf::CircleShape bg(5);
                bg.setFillColor(sf::Color(100, 100, 255, 50));
                bg.setPosition(x, y);
                window.draw(bg);
            }

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
            // 게임 플레이 화면 (흔들림 적용)
            // 보드 그리기
            for(int y=0; y<ROWS; ++y) {
                for(int x=0; x<COLS; ++x) {
                    sf::Color tileColor = getColor(board.g[y][x]);
                    
                    // 레벨업 효과 - 전체 화면 밝게
                    if(board.levelUpEffect > 0) {
                        float intensity = board.levelUpEffect * 0.5f;
                        tileColor.r = std::min(255, (int)(tileColor.r + 100 * intensity));
                        tileColor.g = std::min(255, (int)(tileColor.g + 100 * intensity));
                        tileColor.b = std::min(255, (int)(tileColor.b + 50 * intensity));
                    }
                    
                    tile.setFillColor(tileColor);
                    tile.setPosition((float)x*CELL+0.5f + shakeOffset.x, (float)y*CELL+0.5f + shakeOffset.y);
                    window.draw(tile);
                }
            }

            // 현재 조각 그리기
            if(alive) {
                auto drawPuyo = [&](int x, int y, Color c) {
                    if(inBounds(x, y)) {
                        tile.setFillColor(getColor(c));
                        tile.setPosition((float)x*CELL+0.5f + shakeOffset.x, (float)y*CELL+0.5f + shakeOffset.y);
                        window.draw(tile);
                    }
                };
                
                drawPuyo(cur.pivot.x, cur.pivot.y, cur.c1);
                drawPuyo(cur.pivot.x + cur.sub.x, cur.pivot.y + cur.sub.y, cur.c2);
            }

            // 파티클 렌더링
            for(const auto& particle : board.particles) {
                particleShape.setRadius(particle.size);
                particleShape.setFillColor(particle.color);
                particleShape.setPosition(particle.position.x - particle.size, 
                                        particle.position.y - particle.size);
                window.draw(particleShape);
            }

            // 점수 이펙트 렌더링
            if(fontLoaded) {
                for(const auto& effect : board.scoreEffects) {
                    sf::Text scoreText("+" + to_string(effect.score), font, 14);
                    scoreText.setFillColor(effect.color);
                    scoreText.setPosition(effect.position);
                    window.draw(scoreText);
                }
            }

            // UI 정보
            if(fontLoaded) {
                int uiX = COLS * CELL + 10;
                
                sf::Text scoreText("Score: " + to_string(board.score), font, 16);
                scoreText.setFillColor(sf::Color::White);
                scoreText.setPosition(uiX, 50);
                window.draw(scoreText);

                sf::Text levelText("Level: " + to_string(board.level) + "/20", font, 16);
                levelText.setFillColor(board.level < 10 ? sf::Color::White : 
                                     board.level < 15 ? sf::Color::Yellow : sf::Color::Red);
                levelText.setPosition(uiX, 80);
                window.draw(levelText);

                int nextLevelScore = (board.level * 500);
                int remainingScore = nextLevelScore - board.score;
                if(remainingScore > 0 && board.level < 20) {
                    sf::Text progressText("Next: " + to_string(remainingScore), font, 12);
                    progressText.setFillColor(LIGHT_GRAY);
                    progressText.setPosition(uiX, 100);
                    window.draw(progressText);
                } else if(board.level >= 20) {
                    sf::Text maxText("MAX LEVEL!", font, 12);
                    maxText.setFillColor(sf::Color::Red);
                    maxText.setPosition(uiX, 100);
                    window.draw(maxText);
                }

                // 다음 조각 미리보기
                sf::Text nextText("Next:", font, 14);
                nextText.setFillColor(sf::Color::Cyan);
                nextText.setPosition(uiX, 120);
                window.draw(nextText);

                // 작은 타일로 다음 조각 그리기 (발광 효과)
                sf::RectangleShape smallTile(sf::Vector2f(16, 16));
                
                // 발광 효과를 위한 배경
                sf::RectangleShape glowTile(sf::Vector2f(20, 20));
                sf::Color glowColor = getColor(nextPair.c1);
                glowColor.a = 100;
                glowTile.setFillColor(glowColor);
                glowTile.setPosition(uiX + 8, 148);
                window.draw(glowTile);
                
                smallTile.setFillColor(getColor(nextPair.c1));
                smallTile.setPosition(uiX + 10, 150);
                window.draw(smallTile);

                glowColor = getColor(nextPair.c2);
                glowColor.a = 100;
                glowTile.setFillColor(glowColor);
                glowTile.setPosition(uiX + 8, 168);
                window.draw(glowTile);

                smallTile.setFillColor(getColor(nextPair.c2));
                smallTile.setPosition(uiX + 10, 170);
                window.draw(smallTile);

                // 레벨업 텍스트 효과
                if(board.levelUpEffect > 0) {
                    sf::Text levelUpText("LEVEL UP!", font, 24);
                    levelUpText.setFillColor(sf::Color::Yellow);
                    float alpha = board.levelUpEffect * 127.5f;
                    sf::Color levelUpColor = levelUpText.getFillColor();
                    levelUpColor.a = static_cast<sf::Uint8>(alpha);
                    levelUpText.setFillColor(levelUpColor);
                    levelUpText.setPosition(uiX - 20, 200);
                    window.draw(levelUpText);
                }

                // 게임 상태 표시
                sf::Text escText("ESC: Menu", font, 12);
                escText.setFillColor(GRAY);
                escText.setPosition(uiX, WINDOW_HEIGHT - 30);
                window.draw(escText);
            }
        }

        window.display();
    }
    
    return 0;
}