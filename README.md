# 📌 Puyo Game (뿌요뿌요 클론 프로젝트)

---

## 🎮 프로젝트 소개 | プロジェクト紹介 | Project Overview

* **KO:** 이 프로젝트는 고전 퍼즐 게임 **뿌요뿌요(Puyo Puyo)** 를 C++과 **SFML(Simple and Fast Multimedia Library)** 로 구현한 개인 학습 프로젝트입니다.
* **JP:** このプロジェクトは、クラシックなパズルゲーム **ぷよぷよ** を C++ と **SFML** を用いて実装した個人学習プロジェクトです。
* **EN:** This project is a personal study project implementing the classic puzzle game **Puyo Puyo** using C++ and **SFML**.

---

## 🛠 기술 스택 | 技術スタック | Tech Stack

* **언어 | 言語 | Language**: C++17
* **라이브러리 | ライブラリ | Library**: [SFML 2.5.1](https://www.sfml-dev.org/)
* **개발 환경 | 開発環境 | Development Environment**: Visual Studio Code + MinGW / MSYS2
* **버전 관리 | バージョン管理 | Version Control**: Git & GitHub

---

## ✨ 주요 기능 | 主な機能 | Key Features

* **KO:** 기본 블록 드롭 및 충돌 판정

* **JP:** 基本的なブロック落下と衝突判定

* **EN:** Basic block drop and collision detection

* **KO:** 4개 이상 연결 시 제거되는 매칭 시스템

* **JP:** 4つ以上のブロックがつながると消えるマッチングシステム

* **EN:** Matching system where 4 or more connected blocks disappear

* **KO:** 점수 계산 및 연쇄(Combo) 기능

* **JP:** スコア計算と連鎖機能

* **EN:** Score calculation and chain (combo) system

* **KO:** 간단한 UI와 게임 루프 구현

* **JP:** シンプルなUIとゲームループの実装

* **EN:** Simple UI and game loop implementation

---

## 📸 실행 화면 | 実行画面 | Screenshots

<p align="center">
  <img src="https://github.com/user-attachments/assets/eb9e5d27-ba6b-4b6f-b54b-8f65ef26b0be" alt="Puyo_1" width="250"/>
  <img src="https://github.com/user-attachments/assets/ef145c17-77e8-4993-8922-cc534a29acc1" alt="Puyo_2" width="250"/>
  <img src="https://github.com/user-attachments/assets/8f761123-c736-4909-8f34-50b49eb2e386" alt="Puyo_3" width="250"/>
</p>

---

## ⚙️ 실행 방법 | 実行方法 | How to Run

### 1. 개발 환경 준비 | 開発環境準備 | Setup Environment

* [MSYS2](https://www.msys2.org/) 설치
* MINGW64 환경 실행 후 다음 패키지 설치:

```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-sfml
```

### 2. 저장소 클론 | リポジトリのクローン | Clone Repository

```bash
git clone https://github.com/Gyu1234/Puyo_Game.git
cd Puyo_Game
```

### 3. 빌드 | ビルド | Build

```bash
g++ src/main.cpp -o puyo -lsfml-graphics -lsfml-window -lsfml-system
```

### 4. 실행 | 実行 | Run

```bash
./puyo
```

➡️ **주의 | 注意 | Note**: 위 명령어는 반드시 **MSYS2 MINGW64 터미널**에서 실행해야 정상 동작합니다.

---

## 🚀 프로젝트 성과 | 成果 | Achievements

* **KO:** C++ 객체지향 프로그래밍 실습

* **JP:** C++ のオブジェクト指向プログラミング実習

* **EN:** Practiced object-oriented programming in C++

* **KO:** 게임 루프 및 이벤트 처리 이해

* **JP:** ゲームループとイベント処理の理解

* **EN:** Gained understanding of game loop and event handling

* **KO:** SFML 활용 경험

* **JP:** SFML の活用経験

* **EN:** Experience with SFML library

* **KO:** 기초적인 게임 알고리즘 설계 능력 향상

* **JP:** 基本的なゲームアルゴリズム設計力の向上

* **EN:** Improved basic game algorithm design skills

---

## 📖 앞으로의 개선 방향 | 今後の改善点 | Future Improvements

* 2인 플레이 지원 | 2人プレイ対応 | Two-player mode
* UI/UX 개선 | UI/UX 改善 | Improved UI/UX
* CMake 빌드 시스템 도입 | CMake 導入 | CMake build system
* GitHub Actions CI/CD 구축 | GitHub Actions CI/CD 構築 | GitHub Actions CI/CD

---

## 🌐 GitHub Repository

🔗 [Puyo\_Game](https://github.com/Gyu1234/Puyo_Game)
