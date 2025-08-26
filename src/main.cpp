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
#include <unordered_map>

using namespace std;

// ---- 화면 비율 개선된 상수들 ----
static const int COLS = 6;
static const int ROWS = 12;
static const int BASE_CELL_SIZE = 32;
static const float ASPECT_RATIO = 4.0f / 3.0f; // 게임의 기본 비율
static const int BASE_GAME_WIDTH = COLS * BASE_CELL_SIZE;
static const int BASE_GAME_HEIGHT = ROWS * BASE_CELL_SIZE;
static const int BASE_UI_WIDTH = 280; // UI 패널 너비 증가
static const int BASE_WINDOW_WIDTH = BASE_GAME_WIDTH + BASE_UI_WIDTH;
static const int BASE_WINDOW_HEIGHT = BASE_GAME_HEIGHT + 80;

// 동적 크기 관리 구조체
struct DisplaySettings {
    float scaleFactor = 1.0f;
    int cellSize = BASE_CELL_SIZE;
    int gameWidth = BASE_GAME_WIDTH;
    int gameHeight = BASE_GAME_HEIGHT;
    int uiWidth = BASE_UI_WIDTH;
    int windowWidth = BASE_WINDOW_WIDTH;
    int windowHeight = BASE_WINDOW_HEIGHT;
    
    void updateScale(int windowW, int windowH) {
        float scaleX = static_cast<float>(windowW) / BASE_WINDOW_WIDTH;
        float scaleY = static_cast<float>(windowH) / BASE_WINDOW_HEIGHT;
        
        // 비율을 유지하면서 더 작은 스케일 사용
        scaleFactor = std::min(scaleX, scaleY);
        
        cellSize = static_cast<int>(BASE_CELL_SIZE * scaleFactor);
        gameWidth = COLS * cellSize;
        gameHeight = ROWS * cellSize;
        uiWidth = static_cast<int>(BASE_UI_WIDTH * scaleFactor);
        windowWidth = gameWidth + uiWidth;
        windowHeight = gameHeight + static_cast<int>(80 * scaleFactor);
    }
    
    sf::Vector2f getGameOffset(int windowW, int windowH) const {
        float offsetX = (windowW - windowWidth) / 2.0f;
        float offsetY = (windowH - windowHeight) / 2.0f;
        return sf::Vector2f(std::max(0.0f, offsetX), std::max(0.0f, offsetY));
    }
};

// 향상된 폰트 매니저
class FontManager {
private:
    std::unordered_map<std::string, sf::Font> fonts;
    sf::Font defaultFont;
    bool defaultFontLoaded = false;
    
public:
    bool loadAllFonts() {
        // 폰트 경로들을 우선순위별로 정리
        vector<pair<string, vector<string>>> fontCategories = {
            {"title", {
                "fonts/orbitron-bold.ttf",
                "fonts/audiowide.ttf", 
                "assets/fonts/title.ttf",
                "C:/Windows/Fonts/impact.ttf",
                "C:/Windows/Fonts/arial.ttf"
            }},
            {"ui", {
                "fonts/roboto.ttf",
                "fonts/opensans.ttf",
                "assets/fonts/ui.ttf", 
                "C:/Windows/Fonts/segoeui.ttf",
                "C:/Windows/Fonts/arial.ttf"
            }},
            {"score", {
                "fonts/courier-new.ttf",
                "fonts/sourcecodepro.ttf",
                "assets/fonts/mono.ttf",
                "C:/Windows/Fonts/consola.ttf", 
                "C:/Windows/Fonts/arial.ttf"
            }},
            {"retro", {
                "fonts/pressstart2p.ttf",
                "fonts/pixelated.ttf",
                "assets/fonts/retro.ttf",
                "C:/Windows/Fonts/arial.ttf"
            }}
        };
        
        // 각 카테고리별 폰트 로딩 시도
        int loadedCount = 0;
        for(const auto& category : fontCategories) {
            for(const auto& path : category.second) {
                sf::Font font;
                if(font.loadFromFile(path)) {
                    fonts[category.first] = font;
                    loadedCount++;
                    break;
                }
            }
        }
        
        // 기본 폰트 로딩 (fallback)
        vector<string> defaultPaths = {
            "C:/Windows/Fonts/arial.ttf",
            "C:/Windows/Fonts/calibri.ttf", 
            "/System/Library/Fonts/Helvetica.ttc",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "arial.ttf"
        };
        
        for(const auto& path : defaultPaths) {
            if(defaultFont.loadFromFile(path)) {
                defaultFontLoaded = true;
                break;
            }
        }
        
        return loadedCount > 0 || defaultFontLoaded;
    }
    
    const sf::Font& getFont(const string& category) const {
        auto it = fonts.find(category);
        if(it != fonts.end()) {
            return it->second;
        }
        return defaultFontLoaded ? defaultFont : fonts.begin()->second;
    }
    
    bool hasFont(const string& category) const {
        return fonts.find(category) != fonts.end();
    }
    
    bool isLoaded() const {
        return !fonts.empty() || defaultFontLoaded;
    }
};

// 게임 상태
enum GameState { MENU, PLAYING, GAME_OVER, PAUSED };

// 셀 상태  
enum Color { EMPTY=0, RED, GREEN, BLUE, YELLOW, PURPLE, COLOR_COUNT };

// 2차원 좌표
struct Vec2 { int x, y; };

// 향상된 텍스트 렌더러 클래스
class TextRenderer {
private:
    const FontManager& fontManager;
    const DisplaySettings& display;
    
public:
    TextRenderer(const FontManager& fm, const DisplaySettings& ds) 
        : fontManager(fm), display(ds) {}
    
    enum TextStyle {
        NORMAL,
        OUTLINED,
        SHADOWED, 
        GLOWING,
        RETRO
    };
    
    void drawText(sf::RenderWindow& window, const string& text, 
                  const string& fontCategory, int baseSize,
                  sf::Vector2f position, sf::Color color,
                  TextStyle style = NORMAL, float scale = 1.0f,
                  sf::Vector2f gameOffset = sf::Vector2f(0, 0)) const {
        
        if(!fontManager.isLoaded()) return;
        
        int scaledSize = static_cast<int>(baseSize * display.scaleFactor * scale);
        sf::Vector2f scaledPos = sf::Vector2f(
            position.x * display.scaleFactor + gameOffset.x,
            position.y * display.scaleFactor + gameOffset.y
        );
        
        sf::Text textObj(text, fontManager.getFont(fontCategory), scaledSize);
        
        switch(style) {
            case SHADOWED: {
                sf::Text shadow = textObj;
                shadow.setFillColor(sf::Color(0, 0, 0, 120));
                shadow.setPosition(scaledPos.x + 2 * display.scaleFactor, 
                                 scaledPos.y + 2 * display.scaleFactor);
                window.draw(shadow);
                break;
            }
            case OUTLINED: {
                textObj.setOutlineThickness(1.5f * display.scaleFactor);
                textObj.setOutlineColor(sf::Color(0, 0, 0, 200));
                break;
            }
            case GLOWING: {
                // 글로우 효과를 위한 여러 레이어
                for(int i = 1; i <= 3; i++) {
                    sf::Text glow = textObj;
                    sf::Color glowColor = color;
                    glowColor.a = static_cast<sf::Uint8>(60 / i);
                    glow.setFillColor(glowColor);
                    float offset = i * 2.0f * display.scaleFactor;
                    for(int dx = -1; dx <= 1; dx++) {
                        for(int dy = -1; dy <= 1; dy++) {
                            if(dx == 0 && dy == 0) continue;
                            glow.setPosition(scaledPos.x + dx * offset, 
                                           scaledPos.y + dy * offset);
                            window.draw(glow);
                        }
                    }
                }
                break;
            }
            case RETRO: {
                // 레트로 스타일: 픽셀화된 효과
                textObj.setOutlineThickness(1.0f * display.scaleFactor);
                textObj.setOutlineColor(sf::Color::Black);
                break;
            }
        }
        
        textObj.setFillColor(color);
        textObj.setPosition(scaledPos);
        window.draw(textObj);
    }
    
    void drawCenteredText(sf::RenderWindow& window, const string& text,
                         const string& fontCategory, int baseSize,
                         sf::Vector2f centerPos, sf::Color color,
                         TextStyle style = NORMAL, float scale = 1.0f,
                         sf::Vector2f gameOffset = sf::Vector2f(0, 0)) const {
        
        if(!fontManager.isLoaded()) return;
        
        int scaledSize = static_cast<int>(baseSize * display.scaleFactor * scale);
        sf::Text textObj(text, fontManager.getFont(fontCategory), scaledSize);
        sf::FloatRect bounds = textObj.getLocalBounds();
        
        sf::Vector2f adjustedPos(
            centerPos.x - bounds.width / 2,
            centerPos.y - bounds.height / 2
        );
        
        drawText(window, text, fontCategory, baseSize, adjustedPos, color, style, scale, gameOffset);
    }
};

// 키 입력 상태 관리 (기존과 동일)
struct InputState {
    bool isPressed = false;
    bool wasPressed = false;
    float timer = 0.0f;
    bool isRepeating = false;
    
    static constexpr float INITIAL_DELAY = 0.25f;
    static constexpr float REPEAT_DELAY = 0.06f;
    
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

// 파티클 시스템 (기존과 동일하지만 스케일 적용)
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
        
        float alpha = std::pow(life / maxLife, 0.7f) * 255.0f;
        color.a = static_cast<sf::Uint8>(std::max(0.0f, alpha));
        
        velocity.y += gravity * dt;
        size *= 0.995f;
        
        return life > 0;
    }
};

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
        
        bounce += dt * 8.0f;
        velocity.y += 30.0f * dt;
        
        if (life > maxLife * 0.8f) {
            scale += dt * 3.0f;
        } else if (life < maxLife * 0.3f) {
            scale -= dt * 1.5f;
        }
        scale = std::max(0.1f, scale);
        
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
    float animationTimer = 0.0f;
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

// 보드 클래스 (스케일링 적용)
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
    
    const DisplaySettings& display;
    
    Board(const DisplaySettings& ds) : display(ds) { 
        clear(); 
        particles.reserve(200);
        scoreEffects.reserve(50);
    }

    void clear() {
        for(int y = 0; y < ROWS; ++y) 
            for(int x = 0; x < COLS; ++x) 
                g[y][x] = EMPTY;
        score = 0; chain = 0; level = 1; totalLinesCleared = 0; combo = 0;
        particles.clear(); scoreEffects.clear();
        screenShake = 0.0f; levelUpEffect = 0.0f; chainDisplayTimer = 0.0f;
        currentChain = 0; comboTimer = 0.0f;
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
    }

    void applyGravity() {
        for(int x = 0; x < COLS; ++x) {
            int write = ROWS - 1;
            for(int y = ROWS - 1; y >= 0; --y) {
                if(g[y][x] != EMPTY) {
                    Color c = g[y][x];
                    g[y][x] = EMPTY;
                    g[write][x] = c;
                    write--;
                }
            }
        }
    }
    
    sf::Color getPuyoColor(Color c) const {
        switch(c) {
            case RED:    return sf::Color(255, 69, 58);
            case GREEN:  return sf::Color(52, 199, 89);
            case BLUE:   return sf::Color(0, 122, 255);
            case YELLOW: return sf::Color(255, 214, 10);
            case PURPLE: return sf::Color(191, 90, 242);
            case EMPTY:  return sf::Color(20, 20, 30);
            default:     return sf::Color::White;
        }
    }
    
    void createExplosionEffect(int x, int y, Color color) {
        sf::Vector2f center(
            static_cast<float>(x * display.cellSize + display.cellSize/2), 
            static_cast<float>(y * display.cellSize + display.cellSize/2)
        );
        sf::Color particleColor = getPuyoColor(color);
        
        int particleCount = 15;
        particles.reserve(particles.size() + particleCount);
        
        for(int i = 0; i < particleCount; i++) {
            float angle = (2 * 3.14159f * i) / particleCount + randomFloat(-0.3f, 0.3f);
            float speed = randomFloat(100, 180) * display.scaleFactor;
            sf::Vector2f velocity(cos(angle) * speed, sin(angle) * speed);
            
            particles.emplace_back(center, velocity, particleColor, 
                                 randomFloat(1.2f, 2.5f), randomFloat(4, 8) * display.scaleFactor);
        }
        
        screenShake = std::max(screenShake, 0.5f);
    }
    
    void createScoreEffect(int x, int y, int points, int chainIndex) {
        sf::Vector2f position(
            static_cast<float>(x * display.cellSize + display.cellSize/2), 
            static_cast<float>(y * display.cellSize + display.cellSize/2)
        );
        sf::Color color = sf::Color::White;
        
        if(chainIndex >= 5) color = sf::Color::Magenta;
        else if(chainIndex >= 4) color = sf::Color::Red;
        else if(chainIndex >= 3) color = sf::Color(255, 165, 0);
        else if(chainIndex >= 2) color = sf::Color::Yellow;
        else if(points > 300) color = sf::Color::Cyan;
        
        scoreEffects.emplace_back(position, points, color);
    }

    int calculateScore(int removed, int chainIndex, int groupCount) {
        int baseScore = removed * removed * 20;
        int chainBonus = 0;
        if(chainIndex >= 2) {
            chainBonus = (1 << (chainIndex-1)) * 120;
        }
        int colorBonus = groupCount > 1 ? groupCount * groupCount * 100 : 0;
        int massBonus = removed >= 10 ? (removed - 9) * 80 : 0;
        int levelBonus = level * 10;
        
        return baseScore + chainBonus + colorBonus + massBonus + levelBonus;
    }

    // popGroupsAndScore, getFallSpeed, isGameOver, updateEffects 메서드들은 기존과 동일...
    
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
            int totalPoints = calculateScore(removedTotal, chainIndex, groupCount);
            score += totalPoints;
            combo++;
            comboTimer = 4.0f;
            totalLinesCleared += groupCount;
            
            if(chainIndex > 1) {
                currentChain = chainIndex;
                chainDisplayTimer = 2.5f;
            }
            
            if(!removedPositions.empty()) {
                Vec2 center = removedPositions[removedPositions.size()/2];
                createScoreEffect(center.x, center.y, totalPoints, chainIndex);
            }
            
            int newLevel = std::min(25, (score / 1200) + 1);
            if(newLevel > level) {
                level = newLevel;
                score += level * 150;
                levelUpEffect = 4.0f;
                
                for(int i = 0; i < 80; i++) {
                    float angle = randomFloat(0, 2 * 3.14159f);
                    float speed = randomFloat(200, 400) * display.scaleFactor;
                    sf::Vector2f pos(
                        static_cast<float>(display.gameWidth / 2), 
                        static_cast<float>(display.gameHeight / 2)
                    );
                    sf::Vector2f vel(cos(angle) * speed, sin(angle) * speed);
                    particles.emplace_back(pos, vel, sf::Color(255, 215, 0), 3.0f, 12 * display.scaleFactor);
                }
            }
        } else {
            combo = 0;
        }
        
        return removedTotal;
    }
    
    float getFallSpeed() const {
        const float speeds[] = {
            1.2f, 1.0f, 0.85f, 0.7f, 0.6f, 0.5f, 0.42f, 0.36f, 0.3f, 0.25f,
            0.22f, 0.19f, 0.16f, 0.14f, 0.12f, 0.1f, 0.085f, 0.07f, 0.06f, 0.05f,
            0.04f, 0.035f, 0.03f, 0.025f, 0.02f
        };
        return speeds[std::min(level-1, 24)];
    }
    
    bool isGameOver() const {
        for(int x = 0; x < COLS; ++x) {
            if(g[1][x] != EMPTY) return true;
        }
        return false;
    }
    
    void updateEffects(float dt) {
        if(!particles.empty()) {
            particles.erase(
                std::remove_if(particles.begin(), particles.end(),
                    [dt](Particle& p) { return !p.update(dt); }),
                particles.end()
            );
        }
        
        if(!scoreEffects.empty()) {
            scoreEffects.erase(
                std::remove_if(scoreEffects.begin(), scoreEffects.end(),
                    [dt](ScoreEffect& e) { return !e.update(dt); }),
                scoreEffects.end()
            );
        }
        
        screenShake = std::max(0.0f, screenShake - dt * 2.5f);
        levelUpEffect = std::max(0.0f, levelUpEffect - dt);
        chainDisplayTimer = std::max(0.0f, chainDisplayTimer - dt);
        comboTimer = std::max(0.0f, comboTimer - dt);
    }
    
    sf::Vector2f getShakeOffset() const {
        if(screenShake <= 0) return sf::Vector2f(0, 0);
        
        float intensity = screenShake * 6.0f * display.scaleFactor;
        return sf::Vector2f(
            randomFloat(-intensity, intensity),
            randomFloat(-intensity, intensity)
        );
    }
};

// 회전 함수들
Vec2 rotateCW(const Vec2& v) { return Vec2{ -v.y, v.x }; }
Vec2 rotateCCW(const Vec2& v) { return Vec2{ v.y, -v.x }; }

bool wallKick(Board& b, PuyoPair& p) {
    if(!b.collision(p)) return true;
    
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
    p.pivot = { COLS/2, 0 };
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
    // 초기 윈도우 설정
    DisplaySettings display;
    FontManager fontManager;
    
    // 폰트 로딩
    bool fontsLoaded = fontManager.loadAllFonts();
    
    sf::RenderWindow window(sf::VideoMode(display.windowWidth, display.windowHeight), 
                           "Enhanced Puyo Puyo Pro - Responsive", 
                           sf::Style::Titlebar | sf::Style::Close | sf::Style::Resize);
    window.setFramerateLimit(60);
    window.setVerticalSyncEnabled(true);

    Board board(display);
    GameState gameState = MENU;
    TextRenderer textRenderer(fontManager, display);

    PuyoPair cur = makeSpawnPair();
    PuyoPair nextPair = makeSpawnPair();
    bool alive = true;

    float fallTimer = 0.f;
    InputState leftInput, rightInput, downInput, rotateInput, rotateCCWInput;

    sf::Clock clock;
    float backgroundTime = 0.0f;

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
        backgroundTime += dt;

        // 윈도우 크기 변경 감지 및 스케일 업데이트
        sf::Vector2u currentSize = window.getSize();
        display.updateScale(currentSize.x, currentSize.y);
        sf::Vector2f gameOffset = display.getGameOffset(currentSize.x, currentSize.y);

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
        window.clear(sf::Color(12, 12, 20));
        sf::Vector2f shakeOffset = board.getShakeOffset();

        if(gameState == MENU) {
            // 향상된 배경 애니메이션
            for(int i = 0; i < 40; i++) {
                float phase = backgroundTime * 0.4f + i * 0.2f;
                float x = sin(phase) * 80 * display.scaleFactor + cos(phase * 0.7f) * 40 * display.scaleFactor + currentSize.x/2;
                float y = cos(phase * 0.5f) * 60 * display.scaleFactor + 100 * display.scaleFactor + i * 8 * display.scaleFactor;
                sf::CircleShape bg(randomFloat(4, 12) * display.scaleFactor);
                sf::Color bgColor = board.getPuyoColor(static_cast<Color>((i % 5) + 1));
                bgColor.a = static_cast<sf::Uint8>(60 + sin(phase) * 40);
                bg.setFillColor(bgColor);
                bg.setPosition(x, y);
                window.draw(bg);
            }

            if(fontsLoaded) {
                float pulse = 1.0f + sin(backgroundTime * 3.0f) * 0.1f;
                textRenderer.drawCenteredText(window, "PUYO PUYO", "title", 48, 
                    sf::Vector2f(currentSize.x/2, 80 * display.scaleFactor), 
                    sf::Color(255, 100, 255), TextRenderer::GLOWING, pulse);
                    
                textRenderer.drawCenteredText(window, "ENHANCED PRO", "ui", 20, 
                    sf::Vector2f(currentSize.x/2, 120 * display.scaleFactor), 
                    sf::Color::Cyan, TextRenderer::SHADOWED);
                
                // 깜빡이는 시작 안내
                float blink = sin(backgroundTime * 4.0f);
                if(blink > 0) {
                    textRenderer.drawCenteredText(window, "Press SPACE to Start", "ui", 18, 
                        sf::Vector2f(currentSize.x/2, 170 * display.scaleFactor), 
                        sf::Color::Yellow, TextRenderer::RETRO);
                }
                
                // 컨트롤 안내 - 중앙 정렬
                vector<string> controls = {
                    "Arrow Keys: Move",
                    "Up/Z: Rotate CW", 
                    "X/A: Rotate CCW",
                    "Down: Soft Drop",
                    "ESC: Pause/Menu"
                };
                
                textRenderer.drawText(window, "Controls:", "ui", 16, 
                    sf::Vector2f(50, 220), sf::Color::Cyan, TextRenderer::NORMAL, 1.0f, gameOffset);
                
                for(size_t i = 0; i < controls.size(); i++) {
                    textRenderer.drawText(window, controls[i], "ui", 12, 
                        sf::Vector2f(50, 245 + i * 18), sf::Color::White, TextRenderer::NORMAL, 1.0f, gameOffset);
                }
            }
        } 
        else if(gameState == PAUSED) {
            if(fontsLoaded) {
                textRenderer.drawCenteredText(window, "PAUSED", "title", 40, 
                    sf::Vector2f(currentSize.x/2, currentSize.y/2 - 60), 
                    sf::Color::Yellow, TextRenderer::OUTLINED);
                textRenderer.drawCenteredText(window, "ESC: Continue", "ui", 16, 
                    sf::Vector2f(currentSize.x/2, currentSize.y/2 - 20), 
                    sf::Color::White);
                textRenderer.drawCenteredText(window, "R: Restart", "ui", 16, 
                    sf::Vector2f(currentSize.x/2, currentSize.y/2), 
                    sf::Color::White);
            }
        }
        else if(gameState == GAME_OVER) {
            if(fontsLoaded) {
                float gameOverPulse = 1.0f + sin(backgroundTime * 5.0f) * 0.15f;
                textRenderer.drawCenteredText(window, "GAME OVER", "title", 40, 
                    sf::Vector2f(currentSize.x/2, 100 * display.scaleFactor), 
                    sf::Color::Red, TextRenderer::GLOWING, gameOverPulse);
                
                textRenderer.drawCenteredText(window, "Final Statistics:", "ui", 18, 
                    sf::Vector2f(currentSize.x/2, 150 * display.scaleFactor), 
                    sf::Color::Cyan);
                
                textRenderer.drawCenteredText(window, "Score: " + to_string(board.score), "score", 16, 
                    sf::Vector2f(currentSize.x/2, 175 * display.scaleFactor), 
                    sf::Color::White);
                
                textRenderer.drawCenteredText(window, "Level: " + to_string(board.level) + "/25", "ui", 16, 
                    sf::Vector2f(currentSize.x/2, 195 * display.scaleFactor), 
                    sf::Color::Cyan);
                
                textRenderer.drawCenteredText(window, "Lines: " + to_string(board.totalLinesCleared), "ui", 16, 
                    sf::Vector2f(currentSize.x/2, 215 * display.scaleFactor), 
                    sf::Color::White);
                
                // 등급 시스템
                string grade = "D";
                sf::Color gradeColor = sf::Color::White;
                if(board.score >= 50000) { grade = "S+"; gradeColor = sf::Color::Magenta; }
                else if(board.score >= 30000) { grade = "S"; gradeColor = sf::Color::Red; }
                else if(board.score >= 20000) { grade = "A"; gradeColor = sf::Color::Yellow; }
                else if(board.score >= 10000) { grade = "B"; gradeColor = sf::Color::Cyan; }
                else if(board.score >= 5000) { grade = "C"; gradeColor = sf::Color::Green; }
                
                textRenderer.drawCenteredText(window, "Grade: " + grade, "title", 18, 
                    sf::Vector2f(currentSize.x/2, 245 * display.scaleFactor), 
                    gradeColor, TextRenderer::GLOWING, 1.2f);
                
                textRenderer.drawCenteredText(window, "R: Restart", "ui", 18, 
                    sf::Vector2f(currentSize.x/2, 280 * display.scaleFactor), 
                    sf::Color::Yellow);
                textRenderer.drawCenteredText(window, "ESC: Menu", "ui", 18, 
                    sf::Vector2f(currentSize.x/2, 305 * display.scaleFactor), 
                    sf::Color::Yellow);
            }
        } 
        else {
            // 게임 플레이 화면 - 스케일링 적용
            sf::RectangleShape tile(sf::Vector2f(display.cellSize - 2, display.cellSize - 2));
            
            // 보드 렌더링
            for(int y = 0; y < ROWS; ++y) {
                for(int x = 0; x < COLS; ++x) {
                    sf::Color tileColor = board.getPuyoColor(board.g[y][x]);
                    
                    tile.setFillColor(tileColor);
                    tile.setPosition(
                        x * display.cellSize + 1 + shakeOffset.x + gameOffset.x, 
                        y * display.cellSize + 1 + shakeOffset.y + gameOffset.y
                    );
                    window.draw(tile);
                    
                    // 하이라이트와 그림자 효과
                    if(board.g[y][x] != EMPTY) {
                        sf::CircleShape highlight(display.cellSize / 6.0f);
                        highlight.setFillColor(sf::Color(255, 255, 255, 80));
                        highlight.setPosition(
                            x * display.cellSize + display.cellSize/3.0f + shakeOffset.x + gameOffset.x, 
                            y * display.cellSize + display.cellSize/4.0f + shakeOffset.y + gameOffset.y
                        );
                        window.draw(highlight);
                        
                        sf::RectangleShape shadow(sf::Vector2f(display.cellSize - 4, display.cellSize - 4));
                        sf::Color shadowColor = tileColor;
                        shadowColor.r /= 2; shadowColor.g /= 2; shadowColor.b /= 2;
                        shadow.setFillColor(shadowColor);
                        shadow.setPosition(
                            x * display.cellSize + 3 + shakeOffset.x + gameOffset.x, 
                            y * display.cellSize + 3 + shakeOffset.y + gameOffset.y
                        );
                        window.draw(shadow);
                    }
                }
            }

            // 현재 조각 그리기
            if(alive) {
                auto drawPuyo = [&](int x, int y, Color c, bool isPivot = false) {
                    if(inBounds(x, y)) {
                        float scale = 1.0f;
                        if(cur.animationTimer < 1.0f) {
                            scale = 0.5f + (cur.animationTimer * 0.5f);
                        }
                        
                        if(isPivot) {
                            scale += sin(backgroundTime * 10.0f) * 0.05f;
                        }
                        
                        sf::RectangleShape puyoTile(sf::Vector2f(
                            (display.cellSize - 2) * scale, 
                            (display.cellSize - 2) * scale
                        ));
                        puyoTile.setFillColor(board.getPuyoColor(c));
                        
                        float offsetX = (display.cellSize - (display.cellSize - 2) * scale) / 2;
                        float offsetY = (display.cellSize - (display.cellSize - 2) * scale) / 2;
                        puyoTile.setPosition(
                            x * display.cellSize + 1 + offsetX + shakeOffset.x + gameOffset.x, 
                            y * display.cellSize + 1 + offsetY + shakeOffset.y + gameOffset.y
                        );
                        window.draw(puyoTile);
                        
                        sf::CircleShape activeGlow(display.cellSize / 4.0f * scale);
                        activeGlow.setFillColor(sf::Color(255, 255, 255, 100));
                        activeGlow.setPosition(
                            x * display.cellSize + display.cellSize/3.0f + shakeOffset.x + gameOffset.x, 
                            y * display.cellSize + display.cellSize/3.0f + shakeOffset.y + gameOffset.y
                        );
                        window.draw(activeGlow);
                    }
                };
                
                drawPuyo(cur.pivot.x, cur.pivot.y, cur.c1, true);
                drawPuyo(cur.pivot.x + cur.sub.x, cur.pivot.y + cur.sub.y, cur.c2, false);
            }

            // 파티클 렌더링
            sf::CircleShape particleShape;
            for(const auto& particle : board.particles) {
                particleShape.setRadius(particle.size);
                particleShape.setFillColor(particle.color);
                particleShape.setPosition(
                    particle.position.x - particle.size + shakeOffset.x + gameOffset.x, 
                    particle.position.y - particle.size + shakeOffset.y + gameOffset.y
                );
                window.draw(particleShape);
            }

            // 점수 이펙트 렌더링
            if(fontsLoaded) {
                for(const auto& effect : board.scoreEffects) {
                    float bounce = sin(effect.bounce) * 3.0f;
                    textRenderer.drawText(window, "+" + to_string(effect.score), "score", 14, 
                        sf::Vector2f(effect.position.x, effect.position.y + bounce), 
                        effect.color, TextRenderer::OUTLINED, effect.scale, 
                        sf::Vector2f(shakeOffset.x + gameOffset.x, shakeOffset.y + gameOffset.y));
                }
            }

            // UI 패널 - 스케일링 적용
            sf::RectangleShape uiPanel(sf::Vector2f(display.uiWidth, currentSize.y));
            uiPanel.setFillColor(sf::Color(15, 15, 25, 220));
            uiPanel.setPosition(display.gameWidth + gameOffset.x + 10, gameOffset.y);
            window.draw(uiPanel);

            sf::RectangleShape uiHeader(sf::Vector2f(display.uiWidth, 4 * display.scaleFactor));
            uiHeader.setFillColor(sf::Color::Cyan);
            uiHeader.setPosition(display.gameWidth + gameOffset.x + 10, gameOffset.y);
            window.draw(uiHeader);

            // UI 정보 - 향상된 폰트 적용
            if(fontsLoaded) {
                float uiX = display.gameWidth + gameOffset.x + 20;
                float yPos = gameOffset.y + 15;
                
                textRenderer.drawText(window, "SCORE", "ui", 14, sf::Vector2f(uiX, yPos), sf::Color::Cyan, TextRenderer::SHADOWED);
                yPos += 25 * display.scaleFactor;
                
                textRenderer.drawText(window, to_string(board.score), "score", 20, sf::Vector2f(uiX, yPos), sf::Color::White, TextRenderer::OUTLINED);
                yPos += 40 * display.scaleFactor;

                textRenderer.drawText(window, "LEVEL", "ui", 14, sf::Vector2f(uiX, yPos), sf::Color::Yellow, TextRenderer::SHADOWED);
                yPos += 25 * display.scaleFactor;
                
                sf::Color levelColor = board.level < 8 ? sf::Color::White : 
                                     board.level < 15 ? sf::Color::Yellow : 
                                     board.level < 20 ? sf::Color(255, 165, 0) : sf::Color::Red;
                textRenderer.drawText(window, to_string(board.level) + "/25", "ui", 18, sf::Vector2f(uiX, yPos), levelColor, TextRenderer::OUTLINED);
                yPos += 30 * display.scaleFactor;
                
                // 레벨 프로그레스 바
                int nextLevelScore = (board.level * 1200);
                int currentLevelScore = ((board.level-1) * 1200);
                if(board.level < 25) {
                    float progress = static_cast<float>(board.score - currentLevelScore) / static_cast<float>(nextLevelScore - currentLevelScore);
                    progress = std::min(1.0f, std::max(0.0f, progress));
                    
                    sf::RectangleShape progressBG(sf::Vector2f(180 * display.scaleFactor, 8 * display.scaleFactor));
                    progressBG.setFillColor(sf::Color(40, 40, 50));
                    progressBG.setPosition(uiX, yPos);
                    window.draw(progressBG);
                    
                    sf::RectangleShape progressBar(sf::Vector2f(180 * display.scaleFactor * progress, 8 * display.scaleFactor));
                    progressBar.setFillColor(levelColor);
                    progressBar.setPosition(uiX, yPos);
                    window.draw(progressBar);
                    yPos += 20 * display.scaleFactor;
                    
                    int remainingScore = nextLevelScore - board.score;
                    textRenderer.drawText(window, "Next: " + to_string(remainingScore), "ui", 10, 
                        sf::Vector2f(uiX, yPos), sf::Color(160, 160, 160));
                } else {
                    textRenderer.drawText(window, "MAX LEVEL!", "title", 12, 
                        sf::Vector2f(uiX, yPos), sf::Color::Red, TextRenderer::GLOWING);
                }
                yPos += 25 * display.scaleFactor;

                // 콤보와 연쇄 표시
                if(board.comboTimer > 0 && board.combo > 1) {
                    sf::Color comboColor = board.combo < 5 ? sf::Color::Yellow :
                                         board.combo < 10 ? sf::Color(255, 165, 0) : 
                                         board.combo < 15 ? sf::Color::Red : sf::Color::Magenta;
                    float comboScale = 1.0f + sin(backgroundTime * 8.0f) * 0.1f;
                    textRenderer.drawText(window, to_string(board.combo) + " COMBO!", "retro", 14, 
                        sf::Vector2f(uiX, yPos), comboColor, TextRenderer::GLOWING, comboScale);
                    yPos += 28 * display.scaleFactor;
                }

                if(board.chainDisplayTimer > 0 && board.currentChain > 1) {
                    sf::Color chainColor = board.currentChain < 3 ? sf::Color::Green :
                                         board.currentChain < 5 ? sf::Color::Yellow : 
                                         board.currentChain < 8 ? sf::Color::Red : sf::Color::Magenta;
                    float scale = 1.2f + (board.chainDisplayTimer / 2.5f) * 0.4f;
                    textRenderer.drawText(window, to_string(board.currentChain) + " CHAIN!", "retro", 16, 
                        sf::Vector2f(uiX, yPos), chainColor, TextRenderer::GLOWING, scale);
                    yPos += 35 * display.scaleFactor;
                }

                // 다음 뿌요 미리보기
                yPos += 15 * display.scaleFactor;
                textRenderer.drawText(window, "NEXT", "ui", 12, sf::Vector2f(uiX, yPos), sf::Color::Cyan, TextRenderer::SHADOWED);
                yPos += 25 * display.scaleFactor;
                
                sf::RectangleShape nextBG(sf::Vector2f(60 * display.scaleFactor, 60 * display.scaleFactor));
                nextBG.setFillColor(sf::Color(25, 25, 35));
                nextBG.setOutlineThickness(1 * display.scaleFactor);
                nextBG.setOutlineColor(sf::Color(70, 70, 80));
                nextBG.setPosition(uiX, yPos);
                window.draw(nextBG);
                
                sf::RectangleShape nextTile(sf::Vector2f(22 * display.scaleFactor, 22 * display.scaleFactor));
                
                nextTile.setFillColor(board.getPuyoColor(nextPair.c1));
                nextTile.setPosition(uiX + 19 * display.scaleFactor, yPos + 10 * display.scaleFactor);
                window.draw(nextTile);

                nextTile.setFillColor(board.getPuyoColor(nextPair.c2));
                nextTile.setPosition(uiX + 19 * display.scaleFactor, yPos + 35 * display.scaleFactor);
                window.draw(nextTile);
                yPos += 80 * display.scaleFactor;

                // 통계 정보
                textRenderer.drawText(window, "STATISTICS", "ui", 12, sf::Vector2f(uiX, yPos), sf::Color::Cyan, TextRenderer::SHADOWED);
                yPos += 20 * display.scaleFactor;
                textRenderer.drawText(window, "Groups: " + to_string(board.totalLinesCleared), "ui", 10, 
                    sf::Vector2f(uiX, yPos), sf::Color::White);
                yPos += 18 * display.scaleFactor;

                // 속도 표시
                float speed = board.getFallSpeed();
                int speedPercent = static_cast<int>((1.2f - speed) / 1.2f * 100);
                sf::Color speedColor = speedPercent < 50 ? sf::Color::Green :
                                     speedPercent < 80 ? sf::Color::Yellow : sf::Color::Red;
                textRenderer.drawText(window, "Speed: " + to_string(speedPercent) + "%", "ui", 10, 
                    sf::Vector2f(uiX, yPos), speedColor);
                yPos += 25 * display.scaleFactor;

                // 레벨업 효과
                if(board.levelUpEffect > 0) {
                    textRenderer.drawText(window, "LEVEL UP!", "title", 16, sf::Vector2f(uiX, yPos), 
                        sf::Color::Yellow, TextRenderer::GLOWING);
                }

                // 컨트롤 안내 - 하단
                float controlsY = currentSize.y - 120 * display.scaleFactor;
                textRenderer.drawText(window, "CONTROLS", "ui", 10, sf::Vector2f(uiX, controlsY), 
                    sf::Color(100, 100, 120));
                controlsY += 18 * display.scaleFactor;
                
                vector<pair<string, string>> controls = {
                    {"←→", "Move"},
                    {"↑Z", "Rotate CW"}, 
                    {"XA", "Rotate CCW"},
                    {"↓", "Soft Drop"},
                    {"ESC", "Pause"}
                };
                
                for(const auto& control : controls) {
                    textRenderer.drawText(window, control.first + ": " + control.second, "ui", 8, 
                        sf::Vector2f(uiX, controlsY), sf::Color(100, 100, 120));
                    controlsY += 13 * display.scaleFactor;
                }
            }

            // 게임 경계선
            sf::RectangleShape border;
            border.setFillColor(sf::Color::Transparent);
            border.setOutlineColor(sf::Color(80, 120, 200));
            border.setOutlineThickness(3 * display.scaleFactor);
            border.setSize(sf::Vector2f(display.gameWidth, display.gameHeight));
            border.setPosition(gameOffset.x + shakeOffset.x, gameOffset.y + shakeOffset.y);
            window.draw(border);
            
            // 상단 마스크
            sf::RectangleShape topMask(sf::Vector2f(display.gameWidth, 60 * display.scaleFactor));
            topMask.setFillColor(sf::Color(12, 12, 20, 150));
            topMask.setPosition(gameOffset.x + shakeOffset.x, gameOffset.y + shakeOffset.y);
            window.draw(topMask);
        }

        window.display();
    }
    
    return 0;
}