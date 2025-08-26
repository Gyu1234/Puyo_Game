### 🎮 プロジェクト紹介 | Project Overview

このプロジェクトは、クラシックなパズルゲーム **ぷよぷよ** を **C++** と **SFML** を用いて実装した個人学習プロジェクトです。

-----

### 🛠 技術スタック | Tech Stack

  - **言語 | Language**: C++17
  - **ライブラリ | Library**: SFML 2.5.1
  - **開発環境 | Development Environment**: Visual Studio Code + MinGW / MSYS2
  - **バージョン管理 | Version Control**: Git & GitHub

-----

### ✨ 主要機能 | Key Features

  - 基本的なブロック落下と衝突判定 | Basic block drop and collision detection
  - 4つ以上のブロックがつながると消えるマッチングシステム | Matching system where 4 or more connected blocks disappear
  - スコア計算と連鎖機能 | Score calculation and chain (combo) system
  - シンプルなUIとゲームループの実装 | Simple UI and game loop implementation

-----

### ⚙️ 実行方法 | How to Run

1.  **開発環境準備 | Setup Environment**
      - MSYS2をインストールします。 | Install MSYS2.
      - MINGW64環境を実行し、以下のパッケージをインストールします。 | Run MINGW64 environment and install the following packages:
        ```bash
        pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-sfml
        ```
2.  **リポジトリのクローン | Clone Repository**
    ```bash
    git clone https://github.com/Gyu1234/Puyo_Game.git
    cd Puyo_Game
    ```
3.  **ビルド | Build**
    ```bash
    g++ src/main.cpp -o puyo -lsfml-graphics -lsfml-window -lsfml-system
    ```
4.  **実行 | Run**
    ```bash
    ./puyo
    ```

> ⚠️ **注意 | Note**: 上記のコマンドは、必ずMSYS2 MINGW64ターミナルで実行してください。 | The above command must be run in the MSYS2 MINGW64 terminal to work correctly.

-----

### 🚀 プロジェクト成果 | Achievements

  - C++ のオブジェクト指向プログラミング実習 | Practiced object-oriented programming in C++
  - ゲームループとイベント処理の理解 | Gained understanding of game loop and event handling
  - SFML の活用経験 | Experience with SFML library
  - 基本的なゲームアルゴリズム設計力の向上 | Improved basic game algorithm design skills

-----

### 📖 今後の改善点 | Future Improvements

  - 2人プレイ対応 | Two-player mode
  - UI/UX 改善 | Improved UI/UX
  - CMake 導入 | CMake build system
  - GitHub Actions CI/CD 構築 | GitHub Actions CI/CD

-----

### 🌐 GitHub リポジトリ | Repository

🔗 [Puyo\_Game](https://www.google.com/search?q=https://github.com/Gyu1234/Puyo_Game)
