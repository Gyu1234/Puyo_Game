# 📌 Puyo Game (ぷよぷよクローンプロジェクト / Puyo Puyo Clone Project)

---

## 🎮 Project Overview | プロジェクト紹介

* **JP:** このプロジェクトは、クラシックなパズルゲーム **ぷよぷよ** を C++ と **SFML** を用いて実装した個人学習プロジェクトです。  
* **EN:** This project is a personal study project implementing the classic puzzle game **Puyo Puyo** using C++ and **SFML**.

---

## 🛠 Tech Stack | 技術スタック

* **Language | 言語**: C++17  
* **Library | ライブラリ**: [SFML 2.5.1](https://www.sfml-dev.org/)  
* **Development Environment | 開発環境**: Visual Studio Code + MinGW / MSYS2  
* **Version Control | バージョン管理**: Git & GitHub  

---

## ✨ Key Features | 主な機能

* **JP:** 基本的なブロック落下と衝突判定  
* **EN:** Basic block drop and collision detection  

* **JP:** 4つ以上のブロックがつながると消えるマッチングシステム  
* **EN:** Matching system where 4 or more connected blocks disappear  

* **JP:** スコア計算と連鎖機能  
* **EN:** Score calculation and chain (combo) system  

* **JP:** シンプルなUIとゲームループの実装  
* **EN:** Simple UI and game loop implementation  

---

## 📸 Screenshots | 実行画面

<p align="center">
  <img src="https://github.com/user-attachments/assets/eb9e5d27-ba6b-4b6f-b54b-8f65ef26b0be" alt="Puyo_1" width="250"/>
  <img src="https://github.com/user-attachments/assets/ef145c17-77e8-4993-8922-cc534a29acc1" alt="Puyo_2" width="250"/>
  <img src="https://github.com/user-attachments/assets/8f761123-c736-4909-8f34-50b49eb2e386" alt="Puyo_3" width="250"/>
</p>

---

## ⚙️ How to Run | 実行方法

### 1. Setup Environment | 開発環境準備

* [MSYS2](https://www.msys2.org/) installation  
* Open **MINGW64** and install packages:

```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-sfml
