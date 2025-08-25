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
#include <memory>

using namespace std;

// ---- 기본 상수 ----
static const int COLS = 6;
static const int ROWS = 12;
static const int CELL = 32;
static const int WINDOW_WIDTH = COLS * CELL + 250;
static const int WINDOW_HEIGHT = ROWS * CELL + 50;

// 게임 상태
enum GameState { MENU, PLAYING, GAME_OVER, PAUSED };

// 셀 상태
enum Color { EMPTY=0, RED, GREEN, BLUE, YELLOW, PURPLE, COLOR_COUNT };

// 2차원 좌표
struct Vec2 { int x, y; };

// 키 입력 상태 관리 (DAS 구현용) - 최적화된 버전
struct InputState {
    bool isPressed = false;
    bool wasPressed = false;
    float timer = 0.0f;
    bool isRepeating = false;
    
    // DAS(Delayed Auto Shift) 설정
    static constexpr float INITIAL_DELAY = 0.25f;
    static constexpr float REPEAT_DELAY = 0.06f; // 더 빠른 반복
    
    void update(float dt, bool keyPressed) {
        wasPressed = isPressed;
        isPressed = keyPressed;
        
        if (keyPressed && !wasPressed) {
            timer = INITIAL_DELAY;
            isRepeating = false;
        } else if (keyPressed && wasPressed) {
            timer -= dt;
            if (timer <= 0) {
                isRepeating = true;
                timer = REPEAT_DELAY;
            }
        } else {
            isRepeating = false;
            timer = 0;
        }
    }
    
    bool shouldTrigger() const {
        return (isPressed && !wasPressed) || isRepeating;
    }
};

// 파티클 시스템 최적화
struct Particle {
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::Color color;
    float life;
    float maxLife;
    float size;
    float gravity = 150.0f;
    
    Particle(sf::Vector2f pos, sf::Vector2f vel, sf::Color col, float lifeTime, float particleSize) 
        : position(pos), velocity(vel), color(col), life(lifeTime), maxLife(lifeTime), size(particleSize) {}
        
    bool update(float dt) {
        position += velocity * dt;
        life -= dt;
        
        // 더 부드러운 알파 페이드
        float alpha = std::pow(life / maxLife, 0.7f) * 255.0f;
        color.a = static_cast<sf::Uint8>(std::max(0.0f, alpha));
        
        velocity.y += gravity * dt;
        size *= 0.995f; // 더 천천히 줄어듦
        
        return life > 0;
    }
};

// 점수 이펙트 개선
struct ScoreEffect {
    sf::Vector2f position;
    sf::Vector2f velocity;
    int score;
    float life;
    float maxLife;
    sf::Color color;
    float scale;
    float bounce = 0.0f;
    
    ScoreEffect(sf::Vector2f pos, int points, sf::Color col) 
        : position(pos), velocity(0, -120), score(points), life(3.0f), maxLife(3.0f), color(col), scale(0.8f) {}
        
    bool update(float dt) {
        position += velocity * dt;
        life -= dt;
        
        // 바운스 효과
        bounce += dt * 8.0f;
        velocity.y += 30.0f * dt; // 중력 효과
        
        // 스케일 애니메이션
        if (life > maxLife * 0.8f) {
            scale += dt * 3.0f;
        } else if (life < maxLife * 0.3f) {
            scale -= dt * 1.5f;
        }
        scale = std::max(0.1f, scale);
        
        // 더 부드러운 페이드아웃
        float alpha = std::pow(life / maxLife, 0.5f) * 255.0f;
        color.a = static_cast<sf::Uint8>(std::max(0.0f, alpha));
        
        return life > 0;
    }
};

// 뿌요쌍 구조체
struct PuyoPair {
    Vec2 pivot;
    Vec2 sub;
    Color c1, c2;
    float animationTimer = 0.0f; // 스폰 애니메이션용
};

// 유틸 함수들
bool inBounds(int x, int y) { return x >= 0 && x < COLS && y >= 0 && y < ROWS; }

std::mt19937& rng() {
    static std::mt19937 gen(static_cast<unsigned>(time(nullptr)));
    return gen;
}

Color randomColor() {
    std::uniform_int_distribution<int> dist(1, COLOR_COUNT-1);
    return static_cast<Color>(dist(rng()));
}

float randomFloat(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng());
}

// 개선된 폰트 로딩
bool loadFont(sf::Font& font) {
    vector<string> fontPaths = {
        "arial.ttf",
        "fonts/arial.ttf",
        "assets/fonts/arial.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/calibri.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
    };
    
    for(const auto& path : fontPaths) {
        if(font.loadFromFile(path)) {
            return true;
        }
    }
    return false;
}

// 향상된 텍스트 렌더링 함수
void drawStyledText(sf::RenderWindow& window, const string& text, sf::Font& font, 
                   int size, sf::Vector2f position, sf::Color color, 
                   bool outline = false, bool shadow = false, float scale = 1.0f) {
    sf::Text textObj(text, font, size);
    
    // 그림자 효과 - 더 부드럽게
    if(shadow) {
        sf::Text shadowText = textObj;
        shadowText.setFillColor(sf::Color(0, 0, 0, 120));
        shadowText.setPosition(position.x + 1.5f, position.y + 1.5f);
        shadowText.setScale(scale, scale);
        window.draw(shadowText);
    }
    
    // 외곽선 효과
    if(outline) {
        textObj.setOutlineThickness(1.5f);
        textObj.setOutlineColor(sf::Color(0, 0, 0, 180));
    }
    
    textObj.setFillColor(color);
    textObj.setPosition(position);
    textObj.setScale(scale, scale);
    window.draw(textObj);
}

// 보드 클래스 - 성능 최적화
struct Board {
    array<array<Color, COLS>, ROWS> g{};
    int score = 0;
    int chain = 0;
    int level = 1;
    int totalLinesCleared = 0;
    int combo = 0;
    float comboTimer = 0.0f;
    
    vector<Particle> particles;
    vector<ScoreEffect> scoreEffects;
    float screenShake = 0.0f;
    float levelUpEffect = 0.0f;
    float chainDisplayTimer = 0.0f;
    int currentChain = 0;
    
    // 렌더링 최적화용
    mutable vector<sf::RectangleShape> tileCache;
    mutable bool cacheValid = false;
    
    Board() { 
        clear(); 
        // 타일 캐시 초기화
        tileCache.reserve(ROWS * COLS);
    }

    void clear() {
        for(int y = 0; y < ROWS; ++y) 
            for(int x = 0; x < COLS; ++x) 
                g[y][x] = EMPTY;
        score = 0; chain = 0; level = 1; totalLinesCleared = 0; combo = 0;
        particles.clear(); scoreEffects.clear();
        particles.reserve(200); // 메모리 미리 할당
        scoreEffects.reserve(50);
        screenShake = 0.0f; levelUpEffect = 0.0f; chainDisplayTimer = 0.0f;
        currentChain = 0; comboTimer = 0.0f;
        cacheValid = false;
    }

    bool isEmpty(int x, int y) const {
        return inBounds(x, y) && g[y][x] == EMPTY;
    }

    bool collision(const PuyoPair& p) const {
        if(!inBounds(p.pivot.x, p.pivot.y) || g[p.pivot.y][p.pivot.x] != EMPTY) return true;
        int sx = p.pivot.x + p.sub.x;
        int sy = p.pivot.y + p.sub.y;
        if(!inBounds(sx, sy) || g[sy][sx] != EMPTY) return true;
        return false;
    }

    void lock(const PuyoPair& p) {
        if(inBounds(p.pivot.x, p.pivot.y)) {
            g[p.pivot.y][p.pivot.x] = p.c1;
        }
        int sx = p.pivot.x + p.sub.x;
        int sy = p.pivot.y + p.sub.y;
        if(inBounds(sx, sy)) {
            g[sy][sx] = p.c2;
        }
        cacheValid = false; // 캐시 무효화
    }

    void applyGravity() {
        bool changed = false;
        for(int x = 0; x < COLS; ++x) {
            int write = ROWS - 1;
            for(int y = ROWS - 1; y >= 0; --y) {
                if(g[y][x] != EMPTY) {
                    if(y != write) changed = true;
                    Color c = g[y][x];
                    g[y][x] = EMPTY;
                    g[write][x] = c;
                    write--;
                }
            }
        }
        if(changed) cacheValid = false;
    }
    
    // 향상된 색상 팔레트 - HDR 색상
    sf::Color getPuyoColor(Color c) const {
        switch(c) {
            case RED:    return sf::Color(255, 69, 58);   // 더 선명한 빨강
            case GREEN:  return sf::Color(52, 199, 89);   // 생생한 초록
            case BLUE:   return sf::Color(0, 122, 255);   // 깊은 파랑
            case YELLOW: return sf::Color(255, 214, 10);  // 황금빛 노랑
            case PURPLE: return sf::Color(191, 90, 242);  // 보라
            case EMPTY:  return sf::Color(20, 20, 30);    // 더 어두운 배경
            default:     return sf::Color::White;
        }
    }
    
    // 최적화된 파티클 생성
    void createExplosionEffect(int x, int y, Color color) {
        sf::Vector2f center(static_cast<float>(x * CELL + CELL/2), static_cast<float>(y * CELL + CELL/2));
        sf::Color particleColor = getPuyoColor(color);
        
        // 파티클 수 줄이고 품질 향상
        int particleCount = 15;
        particles.reserve(particles.size() + particleCount);
        
        for(int i = 0; i < particleCount; i++) {
            float angle = (2 * 3.14159f * i) / particleCount + randomFloat(-0.3f, 0.3f);
            float speed = randomFloat(100, 180);
            sf::Vector2f velocity(cos(angle) * speed, sin(angle) * speed);
            
            particles.emplace_back(center, velocity, particleColor, 
                                 randomFloat(1.2f, 2.5f), randomFloat(4, 8));
        }
        
        screenShake = std::max(screenShake, 0.5f);
    }
    
    void createScoreEffect(int x, int y, int points, int chainIndex) {
        sf::Vector2f position(static_cast<float>(x * CELL + CELL/2), static_cast<float>(y * CELL + CELL/2));
        sf::Color color = sf::Color::White;
        
        if(chainIndex >= 5) color = sf::Color::Magenta;
        else if(chainIndex >= 4) color = sf::Color::Red;
        else if(chainIndex >= 3) color = sf::Color(255, 165, 0); // Orange
        else if(chainIndex >= 2) color = sf::Color::Yellow;
        else if(points > 300) color = sf::Color::Cyan;
        
        scoreEffects.emplace_back(position, points, color);
    }

    // 향상된 점수 계산 시스템
    int calculateScore(int removed, int chainIndex, int groupCount) {
        // 기본 점수: 제거된 뿌요 수의 제곱
        int baseScore = removed * removed * 20;
        
        // 연쇄 보너스: 지수적 증가
        int chainBonus = 0;
        if(chainIndex >= 2) {
            chainBonus = (1 << (chainIndex-1)) * 120;
        }
        
        // 동시 소거 보너스
        int colorBonus = groupCount > 1 ? groupCount * groupCount * 100 : 0;
        
        // 대량 소거 보너스
        int massBonus = removed >= 10 ? (removed - 9) * 80 : 0;
        
        // 레벨 보너스
        int levelBonus = level * 10;
        
        return baseScore + chainBonus + colorBonus + massBonus + levelBonus;
    }

    int popGroupsAndScore(int chainIndex) {
        vector<vector<bool>> vis(ROWS, vector<bool>(COLS, false));
        int removedTotal = 0;
        int groupCount = 0;
        vector<Vec2> removedPositions;

        for(int y = 0; y < ROWS; ++y) {
            for(int x = 0; x < COLS; ++x) {
                if(g[y][x] == EMPTY || vis[y][x]) continue;
                Color c = g[y][x];

                vector<Vec2> group;
                queue<Vec2> q;
                q.push({x, y});
                vis[y][x] = true;
                
                while(!q.empty()) {
                    auto [cx, cy] = q.front(); q.pop();
                    group.push_back({cx, cy});
                    const int dx[4] = {1, -1, 0, 0};
                    const int dy[4] = {0, 0, 1, -1};
                    for(int i = 0; i < 4; ++i) {
                        int nx = cx + dx[i], ny = cy + dy[i];
                        if(inBounds(nx, ny) && !vis[ny][nx] && g[ny][nx] == c) {
                            vis[ny][nx] = true;
                            q.push({nx, ny});
                        }
                    }
                }

                if(static_cast<int>(group.size()) >= 4) {
                    for(auto &v : group) {
                        g[v.y][v.x] = EMPTY;
                        removedPositions.push_back(v);
                        createExplosionEffect(v.x, v.y, c);
                    }
                    removedTotal += static_cast<int>(group.size());
                    groupCount++;
                }
            }
        }

        if(removedTotal > 0) {
            cacheValid = false; // 캐시 무효화
            int totalPoints = calculateScore(removedTotal, chainIndex, groupCount);
            score += totalPoints;
            combo++;
            comboTimer = 4.0f; // 콤보 표시 시간 증가
            totalLinesCleared += groupCount;
            
            // 연쇄 표시
            if(chainIndex > 1) {
                currentChain = chainIndex;
                chainDisplayTimer = 2.5f;
            }
            
            if(!removedPositions.empty()) {
                Vec2 center = removedPositions[removedPositions.size()/2];
                createScoreEffect(center.x, center.y, totalPoints, chainIndex);
            }
            
            // 레벨업 체크 - 더 균형잡힌 시스템
            int newLevel = std::min(25, (score / 1200) + 1); // 최대 레벨 25로 증가
            if(newLevel > level) {
                level = newLevel;
                score += level * 150; // 레벨업 보너스 증가
                levelUpEffect = 4.0f;
                
                // 레벨업 파티클 - 더 화려하게
                for(int i = 0; i < 80; i++) {
                    float angle = randomFloat(0, 2 * 3.14159f);
                    float speed = randomFloat(200, 400);
                    sf::Vector2f pos(static_cast<float>(COLS * CELL / 2), static_cast<float>(ROWS * CELL / 2));
                    sf::Vector2f vel(cos(angle) * speed, sin(angle) * speed);
                    particles.emplace_back(pos, vel, sf::Color(255, 215, 0), 3.0f, 12); // Gold
                }
            }
        } else {
            combo = 0; // 콤보 리셋
        }
        
        return removedTotal;
    }
    
    // 부드러운 낙하 속도 곡선
    float getFallSpeed() const {
        const float speeds[] = {
            1.2f, 1.0f, 0.85f, 0.7f, 0.6f, 0.5f, 0.42f, 0.36f, 0.3f, 0.25f,
            0.22f, 0.19f, 0.16f, 0.14f, 0.12f, 0.1f, 0.085f, 0.07f, 0.06f, 0.05f,
            0.04f, 0.035f, 0.03f, 0.025f, 0.02f
        };
        return speeds[std::min(level-1, 24)];
    }
    
    bool isGameOver() const {
        // 더 관대한 게임오버 조건
        for(int x = 0; x < COLS; ++x) {
            if(g[1][x] != EMPTY) return true; // 2행에서 체크
        }
        return false;
    }
    
    // 최적화된 이펙트 업데이트
    void updateEffects(float dt) {
        // 파티클 업데이트 - 벡터 크기 미리 체크
        if(!particles.empty()) {
            particles.erase(
                std::remove_if(particles.begin(), particles.end(),
                    [dt](Particle& p) { return !p.update(dt); }),
                particles.end()
            );
        }
        
        // 점수 이펙트 업데이트
        if(!scoreEffects.empty()) {
            scoreEffects.erase(
                std::remove_if(scoreEffects.begin(), scoreEffects.end(),
                    [dt](ScoreEffect& e) { return !e.update(dt); }),
                scoreEffects.end()
            );
        }
        
        // 화면 흔들림과 기타 이펙트
        screenShake = std::max(0.0f, screenShake - dt * 2.5f);
        levelUpEffect = std::max(0.0f, levelUpEffect - dt);
        chainDisplayTimer = std::max(0.0f, chainDisplayTimer - dt);
        comboTimer = std::max(0.0f, comboTimer - dt);
    }
    
    sf::Vector2f getShakeOffset() const {
        if(screenShake <= 0) return sf::Vector2f(0, 0);
        
        float intensity = screenShake * 6.0f; // 흔들림 강도 줄임
        return sf::Vector2f(
            randomFloat(-intensity, intensity),
            randomFloat(-intensity, intensity)
        );
    }
};

// 회전 함수들
Vec2 rotateCW(const Vec2& v) { return Vec2{ -v.y, v.x }; }
Vec2 rotateCCW(const Vec2& v) { return Vec2{ v.y, -v.x }; }

// 향상된 벽차기 시스템
bool wallKick(Board& b, PuyoPair& p) {
    if(!b.collision(p)) return true;
    
    // 더 많은 벽차기 시도
    vector<Vec2> kickTests = {{-1, 0}, {1, 0}, {-2, 0}, {2, 0}, {0, -1}};
    
    for(const auto& kick : kickTests) {
        PuyoPair test = p;
        test.pivot.x += kick.x;
        test.pivot.y += kick.y;
        if(!b.collision(test)) {
            p = test;
            return true;
        }
    }
    return false;
}

PuyoPair makeSpawnPair() {
    PuyoPair p;
    p.pivot = { COLS/2, 0 }; // 더 위에서 시작
    p.sub = { 0, -1 };
    p.c1 = randomColor();
    p.c2 = randomColor();
    p.animationTimer = 0.0f;
    return p;
}

bool canMove(Board& b, const PuyoPair& p, int dx, int dy) {
    PuyoPair t = p;
    t.pivot.x += dx; t.pivot.y += dy;
    return !b.collision(t);
}

int main() {
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), 
                           "Enhanced Puyo Puyo Pro", 
                           sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);
    window.setVerticalSyncEnabled(true); // VSync로 더 부드러운 렌더링

    sf::Font font;
    bool fontLoaded = loadFont(font);

    Board board;
    GameState gameState = MENU;

    // 렌더링 객체들 미리 생성
    sf::RectangleShape tile(sf::Vector2f(static_cast<float>(CELL-2), static_cast<float>(CELL-2)));
    sf::CircleShape particleShape(3);
    sf::RectangleShape uiPanel(sf::Vector2f(230, static_cast<float>(WINDOW_HEIGHT)));

    PuyoPair cur = makeSpawnPair();
    PuyoPair nextPair = makeSpawnPair();
    bool alive = true;

    float fallTimer = 0.f;
    InputState leftInput, rightInput, downInput, rotateInput, rotateCCWInput;

    sf::Clock clock;
    float backgroundTime = 0.0f;
    
    // 성능 모니터링
    float frameTime = 0.0f;
    int frameCount = 0;

    auto resetGame = [&](){
        board.clear();
        cur = makeSpawnPair();
        nextPair = makeSpawnPair();
        alive = true;
        fallTimer = 0;
        gameState = PLAYING;
    };

    while(window.isOpen()) {
        float dt = clock.restart().asSeconds();
        frameTime += dt;
        frameCount++;
        backgroundTime += dt;

        sf::Event e;
        while(window.pollEvent(e)) {
            if(e.type == sf::Event::Closed) window.close();
            
            if(e.type == sf::Event::KeyPressed) {
                if(gameState == MENU) {
                    if(e.key.code == sf::Keyboard::Space || e.key.code == sf::Keyboard::Return) {
                        resetGame();
                    } else if(e.key.code == sf::Keyboard::Escape) {
                        window.close();
                    }
                } else if(gameState == GAME_OVER) {
                    if(e.key.code == sf::Keyboard::R) {
                        resetGame();
                    } else if(e.key.code == sf::Keyboard::Escape) {
                        gameState = MENU;
                    }
                } else if(gameState == PLAYING) {
                    if(e.key.code == sf::Keyboard::Escape) {
                        gameState = PAUSED;
                    }
                } else if(gameState == PAUSED) {
                    if(e.key.code == sf::Keyboard::Escape) {
                        gameState = PLAYING;
                    } else if(e.key.code == sf::Keyboard::R) {
                        resetGame();
                    }
                }
            }
        }

        // 게임 로직
        if(gameState == PLAYING && alive) {
            cur.animationTimer += dt * 4.0f;
            
            // 향상된 입력 처리
            leftInput.update(dt, sf::Keyboard::isKeyPressed(sf::Keyboard::Left));
            rightInput.update(dt, sf::Keyboard::isKeyPressed(sf::Keyboard::Right));
            downInput.update(dt, sf::Keyboard::isKeyPressed(sf::Keyboard::Down));
            rotateInput.update(dt, sf::Keyboard::isKeyPressed(sf::Keyboard::Up) || 
                                  sf::Keyboard::isKeyPressed(sf::Keyboard::Z));
            rotateCCWInput.update(dt, sf::Keyboard::isKeyPressed(sf::Keyboard::X) ||
                                     sf::Keyboard::isKeyPressed(sf::Keyboard::A));

            if(leftInput.shouldTrigger() && canMove(board, cur, -1, 0)) {
                cur.pivot.x -= 1;
            }
            if(rightInput.shouldTrigger() && canMove(board, cur, +1, 0)) {
                cur.pivot.x += 1;
            }

            // 양방향 회전 지원
            if(rotateInput.shouldTrigger()) {
                PuyoPair t = cur;
                t.sub = rotateCW(t.sub);
                if(board.collision(t)) {
                    wallKick(board, t);
                }
                cur = t;
            }
            
            if(rotateCCWInput.shouldTrigger()) {
                PuyoPair t = cur;
                t.sub = rotateCCW(t.sub);
                if(board.collision(t)) {
                    wallKick(board, t);
                }
                cur = t;
            }

            fallTimer += dt;
            float curInterval = board.getFallSpeed();
            
            // 소프트 드롭
            if(downInput.shouldTrigger()) {
                curInterval = 0.02f;
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

                    if(board.isGameOver()) {
                        alive = false;
                        gameState = GAME_OVER;
                    }
                }
            }
        }

        board.updateEffects(dt);

        // 렌더링
        window.clear(sf::Color(12, 12, 20)); // 더 어두운 배경

        sf::Vector2f shakeOffset = board.getShakeOffset();

        if(gameState == MENU) {
            // 향상된 배경 애니메이션
            for(int i = 0; i < 40; i++) {
                float phase = backgroundTime * 0.4f + i * 0.2f;
                float x = sin(phase) * 80 + cos(phase * 0.7f) * 40 + WINDOW_WIDTH/2;
                float y = cos(phase * 0.5f) * 60 + 100 + i * 8;
                sf::CircleShape bg(randomFloat(4, 12));
                sf::Color bgColor = board.getPuyoColor(static_cast<Color>((i % 5) + 1));
                bgColor.a = static_cast<sf::Uint8>(60 + sin(phase) * 40);
                bg.setFillColor(bgColor);
                bg.setPosition(x, y);
                window.draw(bg);
            }

            if(fontLoaded) {
                // 제목에 펄스 효과
                float pulse = 1.0f + sin(backgroundTime * 3.0f) * 0.1f;
                drawStyledText(window, "PUYO PUYO", font, 48, {WINDOW_WIDTH/2 - 120, 60}, 
                              sf::Color(255, 100, 255), true, true, pulse);
                drawStyledText(window, "ENHANCED PRO", font, 20, {WINDOW_WIDTH/2 - 60, 110}, 
                              sf::Color::Cyan, false, true, 1.0f);
                              
                // 깜빡이는 시작 안내
                float blink = sin(backgroundTime * 4.0f);
                if(blink > 0) {
                    drawStyledText(window, "Press SPACE to Start", font, 18, {WINDOW_WIDTH/2 - 90, 160}, 
                                  sf::Color::Yellow, false, true, 1.0f);
                }
                              
                drawStyledText(window, "Controls:", font, 16, {50, 210}, sf::Color::Cyan);
                drawStyledText(window, "Arrow Keys: Move", font, 12, {50, 230}, sf::Color::White);
                drawStyledText(window, "Up/Z: Rotate CW", font, 12, {50, 245}, sf::Color::White);
                drawStyledText(window, "X/A: Rotate CCW", font, 12, {50, 260}, sf::Color::White);
                drawStyledText(window, "Down: Soft Drop", font, 12, {50, 275}, sf::Color::White);
                drawStyledText(window, "ESC: Pause/Menu", font, 12, {50, 290}, sf::Color::White);
            }
        } 
        else if(gameState == PAUSED) {
            if(fontLoaded) {
                drawStyledText(window, "PAUSED", font, 40, {WINDOW_WIDTH/2 - 70, WINDOW_HEIGHT/2 - 60}, 
                              sf::Color::Yellow, true, true, 1.0f);
                drawStyledText(window, "ESC: Continue", font, 16, {WINDOW_WIDTH/2 - 60, WINDOW_HEIGHT/2 - 20}, 
                              sf::Color::White, false, true, 1.0f);
                drawStyledText(window, "R: Restart", font, 16, {WINDOW_WIDTH/2 - 45, WINDOW_HEIGHT/2}, 
                              sf::Color::White, false, true, 1.0f);
            }
        }
        else if(gameState == GAME_OVER) {
            if(fontLoaded) {
                float gameOverPulse = 1.0f + sin(backgroundTime * 5.0f) * 0.15f;
                drawStyledText(window, "GAME OVER", font, 40, {WINDOW_WIDTH/2 - 100, 100}, 
                              sf::Color::Red, true, true, gameOverPulse);
                
                drawStyledText(window, "Final Statistics:", font, 18, {WINDOW_WIDTH/2 - 80, 150}, 
                              sf::Color::Cyan, false, true, 1.0f);
                drawStyledText(window, "Score: " + to_string(board.score), font, 16, 
                              {WINDOW_WIDTH/2 - 60, 175}, sf::Color::White, false, true, 1.0f);
                drawStyledText(window, "Level: " + to_string(board.level) + "/25", font, 16, 
                              {WINDOW_WIDTH/2 - 60, 195}, sf::Color::Cyan, false, true, 1.0f);
                drawStyledText(window, "Lines: " + to_string(board.totalLinesCleared), font, 16, 
                              {WINDOW_WIDTH/2 - 60, 215}, sf::Color::White, false, true, 1.0f);
                
                // 등급 시스템
                string grade = "D";
                sf::Color gradeColor = sf::Color::White;
                if(board.score >= 50000) { grade = "S+"; gradeColor = sf::Color::Magenta; }
                else if(board.score >= 30000) { grade = "S"; gradeColor = sf::Color::Red; }
                else if(board.score >= 20000) { grade = "A"; gradeColor = sf::Color::Yellow; }
                else if(board.score >= 10000) { grade = "B"; gradeColor = sf::Color::Cyan; }
                else if(board.score >= 5000) { grade = "C"; gradeColor = sf::Color::Green; }
                
                drawStyledText(window, "Grade: " + grade, font, 18, 
                              {WINDOW_WIDTH/2 - 50, 245}, gradeColor, true, true, 1.2f);
                              
                drawStyledText(window, "R: Restart", font, 18, {WINDOW_WIDTH/2 - 50, 280}, 
                              sf::Color::Yellow, false, true, 1.0f);
                drawStyledText(window, "ESC: Menu", font, 18, {WINDOW_WIDTH/2 - 50, 305}, 
                              sf::Color::Yellow, false, true, 1.0f);
            }
        } 
        else {
            // 게임 플레이 화면
            
            // 향상된 보드 렌더링
            for(int y = 0; y < ROWS; ++y) {
                for(int x = 0; x < COLS; ++x) {
                    sf::Color tileColor = board.getPuyoColor(board.g[y][x]);
                    
                    tile.setFillColor(tileColor);
                    tile.setPosition(static_cast<float>(x*CELL+1) + shakeOffset.x, static_cast<float>(y*CELL+1) + shakeOffset.y);
                    window.draw(tile);
                    
                    // 뿌요에 더 나은 하이라이트
                    if(board.g[y][x] != EMPTY) {
                        sf::CircleShape highlight(static_cast<float>(CELL/6));
                        highlight.setFillColor(sf::Color(255, 255, 255, 80));
                        highlight.setPosition(static_cast<float>(x*CELL + CELL/3) + shakeOffset.x, 
                                            static_cast<float>(y*CELL + CELL/4) + shakeOffset.y);
                        window.draw(highlight);
                        
                        // 깊이감을 위한 그림자
                        sf::RectangleShape shadow(sf::Vector2f(static_cast<float>(CELL-4), static_cast<float>(CELL-4)));
                        sf::Color shadowColor = tileColor;
                        shadowColor.r /= 2; shadowColor.g /= 2; shadowColor.b /= 2;
                        shadow.setFillColor(shadowColor);
                        shadow.setPosition(static_cast<float>(x*CELL+3) + shakeOffset.x, 
                                         static_cast<float>(y*CELL+3) + shakeOffset.y);
                        window.draw(shadow);
                    }
                }
            }

            // 현재 조각 그리기 - 애니메이션 추가
            if(alive) {
                auto drawPuyo = [&](int x, int y, Color c, bool isPivot = false) {
                    if(inBounds(x, y)) {
                        // 스폰 애니메이션
                        float scale = 1.0f;
                        if(cur.animationTimer < 1.0f) {
                            scale = 0.5f + (cur.animationTimer * 0.5f);
                        }
                        
                        // 회전할 때의 펄스 효과
                        if(isPivot) {
                            scale += sin(backgroundTime * 10.0f) * 0.05f;
                        }
                        
                        sf::RectangleShape puyoTile(sf::Vector2f(static_cast<float>((CELL-2) * scale), static_cast<float>((CELL-2) * scale)));
                        puyoTile.setFillColor(board.getPuyoColor(c));
                        
                        float offsetX = (CELL - (CELL-2) * scale) / 2;
                        float offsetY = (CELL - (CELL-2) * scale) / 2;
                        puyoTile.setPosition(static_cast<float>(x*CELL+1) + offsetX + shakeOffset.x, 
                                           static_cast<float>(y*CELL+1) + offsetY + shakeOffset.y);
                        window.draw(puyoTile);
                        
                        // 조작중인 뿌요 특별 효과
                        sf::CircleShape activeGlow(static_cast<float>(CELL/4 * scale));
                        activeGlow.setFillColor(sf::Color(255, 255, 255, 100));
                        activeGlow.setPosition(static_cast<float>(x*CELL + CELL/3) + shakeOffset.x, 
                                             static_cast<float>(y*CELL + CELL/3) + shakeOffset.y);
                        window.draw(activeGlow);
                    }
                };
                
                drawPuyo(cur.pivot.x, cur.pivot.y, cur.c1, true);
                drawPuyo(cur.pivot.x + cur.sub.x, cur.pivot.y + cur.sub.y, cur.c2, false);
            }

            // 향상된 파티클 렌더링
            for(const auto& particle : board.particles) {
                particleShape.setRadius(particle.size);
                particleShape.setFillColor(particle.color);
                particleShape.setPosition(particle.position.x - particle.size + shakeOffset.x, 
                                        particle.position.y - particle.size + shakeOffset.y);
                window.draw(particleShape);
            }

            // 점수 이펙트 렌더링
            if(fontLoaded) {
                for(const auto& effect : board.scoreEffects) {
                    float bounce = sin(effect.bounce) * 3.0f;
                    drawStyledText(window, "+" + to_string(effect.score), font, 14, 
                                 {effect.position.x + shakeOffset.x, effect.position.y + shakeOffset.y + bounce}, 
                                 effect.color, true, false, effect.scale);
                }
            }

            // 향상된 UI 패널
            uiPanel.setFillColor(sf::Color(15, 15, 25, 220));
            uiPanel.setPosition(static_cast<float>(COLS * CELL + 10), 0);
            window.draw(uiPanel);

            // 그라디언트 효과를 위한 UI 상단 강조
            sf::RectangleShape uiHeader(sf::Vector2f(230, 4));
            uiHeader.setFillColor(sf::Color::Cyan);
            uiHeader.setPosition(static_cast<float>(COLS * CELL + 10), 0);
            window.draw(uiHeader);

            // UI 정보 - 더욱 향상된 레이아웃
            if(fontLoaded) {
                int uiX = COLS * CELL + 20;
                int yPos = 15;
                
                // 점수 - 애니메이션 효과
                drawStyledText(window, "SCORE", font, 14, {static_cast<float>(uiX), static_cast<float>(yPos)}, sf::Color::Cyan, false, true);
                yPos += 20;
                
                // 점수 증가 애니메이션 (간단화)
                string scoreStr = to_string(board.score);
                drawStyledText(window, scoreStr, font, 20, {static_cast<float>(uiX), static_cast<float>(yPos)}, sf::Color::White, true, true);
                yPos += 35;

                // 레벨 - 프로그레스 바 스타일
                drawStyledText(window, "LEVEL", font, 14, {static_cast<float>(uiX), static_cast<float>(yPos)}, sf::Color::Yellow, false, true);
                yPos += 20;
                
                sf::Color levelColor = board.level < 8 ? sf::Color::White : 
                                     board.level < 15 ? sf::Color::Yellow : 
                                     board.level < 20 ? sf::Color(255, 165, 0) : sf::Color::Red;
                drawStyledText(window, to_string(board.level) + "/25", font, 18, {static_cast<float>(uiX), static_cast<float>(yPos)}, levelColor, true, true);
                yPos += 25;
                
                // 레벨 프로그레스 바
                int nextLevelScore = (board.level * 1200);
                int currentLevelScore = ((board.level-1) * 1200);
                if(board.level < 25) {
                    float progress = static_cast<float>(board.score - currentLevelScore) / static_cast<float>(nextLevelScore - currentLevelScore);
                    progress = std::min(1.0f, std::max(0.0f, progress));
                    
                    sf::RectangleShape progressBG(sf::Vector2f(180, 8));
                    progressBG.setFillColor(sf::Color(40, 40, 50));
                    progressBG.setPosition(static_cast<float>(uiX), static_cast<float>(yPos));
                    window.draw(progressBG);
                    
                    sf::RectangleShape progressBar(sf::Vector2f(180 * progress, 8));
                    progressBar.setFillColor(levelColor);
                    progressBar.setPosition(static_cast<float>(uiX), static_cast<float>(yPos));
                    window.draw(progressBar);
                    yPos += 15;
                    
                    int remainingScore = nextLevelScore - board.score;
                    drawStyledText(window, "Next: " + to_string(remainingScore), font, 10, {static_cast<float>(uiX), static_cast<float>(yPos)}, 
                                 sf::Color(160, 160, 160), false, true);
                } else {
                    drawStyledText(window, "MAX LEVEL!", font, 12, {static_cast<float>(uiX), static_cast<float>(yPos)}, sf::Color::Red, true, true);
                }
                yPos += 20;

                // 콤보와 연쇄 표시 - 더 화려하게
                if(board.comboTimer > 0 && board.combo > 1) {
                    sf::Color comboColor = board.combo < 5 ? sf::Color::Yellow :
                                         board.combo < 10 ? sf::Color(255, 165, 0) : 
                                         board.combo < 15 ? sf::Color::Red : sf::Color::Magenta;
                    float comboScale = 1.0f + sin(backgroundTime * 8.0f) * 0.1f;
                    drawStyledText(window, to_string(board.combo) + " COMBO!", font, 14, {static_cast<float>(uiX), static_cast<float>(yPos)}, 
                                 comboColor, true, true, comboScale);
                    yPos += 22;
                }

                if(board.chainDisplayTimer > 0 && board.currentChain > 1) {
                    sf::Color chainColor = board.currentChain < 3 ? sf::Color::Green :
                                         board.currentChain < 5 ? sf::Color::Yellow : 
                                         board.currentChain < 8 ? sf::Color::Red : sf::Color::Magenta;
                    float scale = 1.2f + (board.chainDisplayTimer / 2.5f) * 0.4f;
                    drawStyledText(window, to_string(board.currentChain) + " CHAIN!", font, 16, {static_cast<float>(uiX), static_cast<float>(yPos)}, 
                                 chainColor, true, true, scale);
                    yPos += 28;
                }

                // 다음 뿌요 미리보기 - 향상된 디자인
                yPos += 10;
                drawStyledText(window, "NEXT", font, 12, {static_cast<float>(uiX), static_cast<float>(yPos)}, sf::Color::Cyan, false, true);
                yPos += 20;
                
                // 다음 뿌요 배경
                sf::RectangleShape nextBG(sf::Vector2f(60, 60));
                nextBG.setFillColor(sf::Color(25, 25, 35));
                nextBG.setOutlineThickness(1);
                nextBG.setOutlineColor(sf::Color(70, 70, 80));
                nextBG.setPosition(static_cast<float>(uiX), static_cast<float>(yPos));
                window.draw(nextBG);
                
                // 다음 뿌요 그리기
                sf::RectangleShape nextTile(sf::Vector2f(22, 22));
                
                nextTile.setFillColor(board.getPuyoColor(nextPair.c1));
                nextTile.setPosition(static_cast<float>(uiX + 19), static_cast<float>(yPos + 10));
                window.draw(nextTile);

                nextTile.setFillColor(board.getPuyoColor(nextPair.c2));
                nextTile.setPosition(static_cast<float>(uiX + 19), static_cast<float>(yPos + 35));
                window.draw(nextTile);
                yPos += 70;

                // 통계 정보 - 더 상세하게
                drawStyledText(window, "STATISTICS", font, 12, {static_cast<float>(uiX), static_cast<float>(yPos)}, sf::Color::Cyan, false, true);
                yPos += 18;
                drawStyledText(window, "Groups: " + to_string(board.totalLinesCleared), font, 10, 
                             {static_cast<float>(uiX), static_cast<float>(yPos)}, sf::Color::White, false, true);
                yPos += 15;

                // 속도 표시 - 더 직관적으로
                float speed = board.getFallSpeed();
                int speedPercent = static_cast<int>((1.2f - speed) / 1.2f * 100);
                sf::Color speedColor = speedPercent < 50 ? sf::Color::Green :
                                     speedPercent < 80 ? sf::Color::Yellow : sf::Color::Red;
                drawStyledText(window, "Speed: " + to_string(speedPercent) + "%", font, 10, 
                             {static_cast<float>(uiX), static_cast<float>(yPos)}, speedColor, false, true);
                yPos += 20;

                // PPS (Pieces Per Second) 추정
                if(board.totalLinesCleared > 0) {
                    float estimatedPPS = board.totalLinesCleared / (backgroundTime / 4.0f); // 대략적 계산
                    drawStyledText(window, "PPS: " + to_string(static_cast<int>(estimatedPPS * 10) / 10.0f), font, 10, 
                                 {static_cast<float>(uiX), static_cast<float>(yPos)}, sf::Color(150, 150, 255), false, true);
                    yPos += 20;
                }

                // 레벨업 효과 텍스트 - 간단하게
                if(board.levelUpEffect > 0) {
                    drawStyledText(window, "LEVEL UP!", font, 16, {static_cast<float>(uiX), static_cast<float>(yPos)}, 
                                 sf::Color::Yellow, true, true);
                }
                yPos += 35;

                // 컨트롤 안내 - 더 깔끔하게
                int controlsY = WINDOW_HEIGHT - 120;
                drawStyledText(window, "CONTROLS", font, 10, {static_cast<float>(uiX), static_cast<float>(controlsY)}, 
                             sf::Color(100, 100, 120), false, true);
                controlsY += 15;
                
                vector<pair<string, string>> controls = {
                    {"←→", "Move"},
                    {"↑Z", "Rotate CW"}, 
                    {"XA", "Rotate CCW"},
                    {"↓", "Soft Drop"},
                    {"ESC", "Pause"}
                };
                
                for(const auto& control : controls) {
                    drawStyledText(window, control.first + ": " + control.second, font, 8, 
                                 {static_cast<float>(uiX), static_cast<float>(controlsY)}, 
                                 sf::Color(100, 100, 120), false, true);
                    controlsY += 11;
                }
            }

            // 게임 경계선 - 더 멋지게
            sf::RectangleShape border;
            border.setFillColor(sf::Color::Transparent);
            border.setOutlineColor(sf::Color(80, 120, 200));
            border.setOutlineThickness(3);
            border.setSize(sf::Vector2f(static_cast<float>(COLS * CELL), static_cast<float>(ROWS * CELL)));
            border.setPosition(0 + shakeOffset.x, 0 + shakeOffset.y);
            window.draw(border);
            
            // 상단에 그라디언트 마스크 (3D 효과)
            sf::RectangleShape topMask(sf::Vector2f(static_cast<float>(COLS * CELL), 60));
            topMask.setFillColor(sf::Color(12, 12, 20, 150));
            topMask.setPosition(0 + shakeOffset.x, 0 + shakeOffset.y);
            window.draw(topMask);
        }

        // FPS 표시 (디버그용, 선택사항)
        if(frameCount >= 60) {
            frameTime = 0;
            frameCount = 0;
        }

        window.display();
    }
    
    return 0;
}