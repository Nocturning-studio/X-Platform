# X-Platform

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-17-blue" alt="C++ Standard">
  <a href="https://github.com/Nocturning-studio/X-Platform/actions/workflows/build.yml">
    <img src="https://github.com/Nocturning-studio/X-Platform/actions/workflows/build.yml/badge.svg" alt="Build Status">
  </a>
  <a href="https://github.com/Nocturning-studio/X-Platform/stargazers">
    <img src="https://img.shields.io/github/stars/Nocturning-studio/X-Platform?style=social" alt="Stars">
  </a>
  <a href="https://discord.gg/XXvxtnDbBP">
    <img src="https://img.shields.io/discord/308323056592486420?logo=discord&logoColor=white" alt="Discord">
  </a>
</p>

X-Platform это проект, нацеленный на переработку X-Ray Engine 1.0, с внедрением современных технологий.

# Структура проекта
```
X-Platform/
├── game_filesystem.ltx
├── appdata/
│   └── user_game_settings.ltx
├── gamedata/
└── Engine/
    ├── Scripts/              ← Скприты для сборки (Билд эвенты)
    ├── Config/               ← Конфиги движка (Не игры)
    ├── Binaries/
    │   └── ($PlatformShortName)/
    │       └── xrEngine.exe
    ├── Shaders/
    └── Sources/
        ├── Runtime/            ← основной код движка (xrCore, xrMath, xrEngine, xrRender и т.д.)
        ├── Third-Party/        ← сторонние библиотеки (ODE, SoftX, PresenceAudio и т.п.)
        └── QOL&Network/        ← утилиты и сетевые инструменты (Лог сендер, лаунчер)
```

# Немного фич проекта
- C++17 и компиляция Visual Studio 2026
- Объемный звук при помощи трассировки пути Presence Audio
- Многопоточный Occlusion culling на базе SoftX
- Интеграция профилировщика Optick
- Полностью переписанный рендер с PBR, IBL, отражениями, симуляцией ветра и многим другим
- Огромный рефакторинг кодовой базы

# Сборка:
- Установите Visual Studio 2026
- Склонируйте репозиторий X-Platform
- Откройте в Visual Studio решение xrEngine.sln
- Выделите в качестве собираемого проекта xrGame и запустите сборку (движок соберется автоматически, а все нужные библиотеки и конфиги скопируются в выходную папку)

# Запуск:
- Скачайте gamedata из репозитория проектов для данного движка (https://github.com/Nocturning-studio/X-Projects)
- Положите содержимое репозитория выше в папку gamedata и положите ее в корневую папку с игрой
- Перейдите в Engine/Binaries/($PlatformShortName)/xrEngine.exe и запустите (Игра сама найдет корневую директорию игры по файлу game_filesystem.ltx)

# Минимальные системные требования
- Процессор: Intel Core I5 2400
- ОЗУ: 4GB
- Видеокарта: GTX750Ti

<img width="3500" height="1876" alt="Splash screen1" src="https://github.com/user-attachments/assets/f4f2b12e-a019-405c-bc74-4fd0d5eff0da" />
<img width="1920" height="1080" alt="Opener" src="https://github.com/user-attachments/assets/b7422e36-c5ce-4792-bb49-75d8ca376bb7" />
<img width="1920" height="1080" alt="PBR" src="https://github.com/user-attachments/assets/ed1a3c22-8c4e-4607-8e72-bfe0a615352b" />
<img width="1920" height="1080" alt="Material" src="https://github.com/user-attachments/assets/f1f760db-ab81-4458-b5e8-a8f5f0fabdc0" />
<img width="1920" height="1080" alt="Scattering" src="https://github.com/user-attachments/assets/29464acf-2e74-458b-a26e-575db36530bb" />
<img width="1920" height="1080" alt="SSPTAO" src="https://github.com/user-attachments/assets/125a28eb-61d0-4b86-ba6d-2dd0c727aedf" />
<img width="1920" height="1080" alt="IBL" src="https://github.com/user-attachments/assets/89675fb2-990e-4962-ade5-574eee3c3ab6" />
<img width="1920" height="1080" alt="Fog" src="https://github.com/user-attachments/assets/7f945ac0-93ee-4eab-a4e3-c0be8dc699c2" />
<img width="1920" height="1080" alt="Closer" src="https://github.com/user-attachments/assets/54900927-8f70-4675-b69b-ed290ec8c3d0" />
