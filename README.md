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
  <img src="https://img.shields.io/badge/License-PolyForm%20Noncommercial%201.0.0-blue.svg" alt="License">
</p>

---

## 📖 Оглавление

- [О проекте](#-о-проекте)
- [Ключевые особенности](#-ключевые-особенности)
- [Структура проекта](#-структура-проекта)
- [Сборка](#-сборка)
- [Запуск](#-запуск)
- [Системные требования](#-системные-требования)
- [Скриншоты](#-скриншоты)
- [Лицензия](#-лицензия)
- [Контакты](#-контакты)

---

## 🚀 О проекте

**X-Platform** — это проект, нацеленный на глубокую переработку движка **X-Ray Engine 1.0** с внедрением современных технологий рендеринга, звука и оптимизации. Цель проекта — дать модмейкерам и игрокам обновлённую, производительную и расширяемую платформу для создания модификаций на базе культовой серии S.T.A.L.K.E.R.

Движок полностью переписан с использованием **C++17** и поддерживает компиляцию в **Visual Studio 2019–2026**.

---

## ✨ Ключевые особенности

| Технология | Описание |
|------------|----------|
| **Современный рендер** | Полностью переписанный рендер с PBR, IBL, отражениями, симуляцией ветра и поддержкой HD-текстур. |
| **Объёмный звук** | Трассировка пути Presence Audio для реалистичного позиционирования звуков. |
| **Occlusion Culling** | Многопоточный Occlusion Culling на базе SoftX для высокой производительности. |
| **Профилирование** | Интеграция профилировщика Optick для анализа узких мест. |
| **Рефакторинг** | Огромная работа по очистке и модернизации кодовой базы. |
| **Гибкость** | Вынесенные шейдеры и модульная архитектура упрощают адаптацию под любые моды. |

---

## 📁 Структура проекта

```
X-Platform/
├── game_filesystem.ltx
├── Appdata/
│   └── user_game_settings.ltx
├── Archives/                 ← Папка с .db* архивами игры
├── Gamedata/                 ← Папка с незапакованными ресурсами вашего мода
├── Engine/
└── Binaries/
    │   └── ($PlatformShortName)/
    │       └── xrEngine.exe    ← Исполняемый файл движка
    ├── Shaders/                ← Шейдеры (для того чтобы не привязывать их к конкретным модам)
    └── Sources/
        ├── Engine/             ← основной код движка (xrCore, xrMath, xrEngine, xrRender и т.д.)
        ├── Game/               ← код игры, игровых модулей, специфичные имплементации игры Third-Party
        ├── Third-Party/        ← сторонние библиотеки (ODE, SoftX, PresenceAudio и т.п.)
        └── QOL&Network/        ← утилиты и сетевые инструменты (Лог сендер, лаунчер)
```

---

## 🔨 Сборка

1. Установите **Visual Studio 2019** (или новее).
2. Склонируйте репозиторий:
   ```bash
   git clone https://github.com/Nocturning-studio/X-Platform.git
   ```
3. Откройте решение `xrEngine.sln` в Visual Studio.
4. Выберите в качестве собираемого проекта **xrGame** и запустите сборку.
   > Движок соберётся автоматически, а все необходимые библиотеки и конфиги скопируются в выходную папку.

---

## ▶️ Запуск

1. Скачайте `gamedata` из репозитория [X-Projects](https://github.com/Nocturning-studio/X-Projects).
2. Поместите содержимое репозитория в папку `gamedata` и положите её в корневую папку с игрой.
3. Создайте папку `Archives` рядом с `Gamedata` и положите в неё `.db*` архивы игры.
4. Запустите `Engine/Binaries/($PlatformShortName)/xrEngine.exe`.

---

## 💻 Системные требования

| Компонент | Минимальные | Рекомендуемые |
|-----------|-------------|---------------|
| **Процессор** | Intel Core i5 2400 / AMD FX 4300 | Intel Core i5 8400 / Ryzen 5 1600 |
| **ОЗУ** | 4 GB | 4 GB |
| **Видеокарта** | GTX 750 Ti | RX 580 |

---

## 🖼️ Скриншоты

<p align="center">
  <img width="1920" height="1080" alt="Splash screen" src="https://github.com/user-attachments/assets/f4f2b12e-a019-405c-bc74-4fd0d5eff0da" />
  <img width="1920" height="1080" alt="Opener" src="https://github.com/user-attachments/assets/b7422e36-c5ce-4792-bb49-75d8ca376bb7" />
  <img width="1920" height="1080" alt="PBR" src="https://github.com/user-attachments/assets/ed1a3c22-8c4e-4607-8e72-bfe0a615352b" />
  <img width="1920" height="1080" alt="Material" src="https://github.com/user-attachments/assets/f1f760db-ab81-4458-b5e8-a8f5f0fabdc0" />
  <img width="1920" height="1080" alt="Scattering" src="https://github.com/user-attachments/assets/29464acf-2e74-458b-a26e-575db36530bb" />
  <img width="1920" height="1080" alt="SSPTAO" src="https://github.com/user-attachments/assets/125a28eb-61d0-4b86-ba6d-2dd0c727aedf" />
  <img width="1920" height="1080" alt="IBL" src="https://github.com/user-attachments/assets/89675fb2-990e-4962-ade5-574eee3c3ab6" />
  <img width="1920" height="1080" alt="Fog" src="https://github.com/user-attachments/assets/7f945ac0-93ee-4eab-a4e3-c0be8dc699c2" />
  <img width="1920" height="1080" alt="Closer" src="https://github.com/user-attachments/assets/54900927-8f70-4675-b69b-ed290ec8c3d0" />
</p>

---

## 📄 Лицензия

Проект распространяется на условиях **PolyForm Noncommercial 1.0.0**.

- **X-Platform Code** (код, созданный Nocturning Studio и независимыми контрибьюторами) — под лицензией PolyForm Noncommercial 1.0.0. Коммерческое использование запрещено без отдельного разрешения.
- **GSC Game World Proprietary Code** — оригинальный код X-Ray Engine остаётся под проприетарной лицензией GSC Game World.
- **Third-Party Components** — сторонние библиотеки распространяются на условиях их собственных лицензий.

Полный текст лицензии доступен в файле [LICENSE](LICENSE) или на сайте [PolyForm Project](https://polyformproject.org/licenses/noncommercial/1.0.0).

---

## 📬 Контакты

- **Discord**: [Присоединиться к серверу](https://discord.gg/XXvxtnDbBP)
- **GitHub Issues**: [Создать issue](https://github.com/Nocturning-studio/X-Platform/issues)

---

## 🙏 Благодарности

- **GSC Game World** — за оригинальный X-Ray Engine.
- **Nocturning Studio** — за координацию и развитие проекта.
- **Все контрибьюторы** — за вклад в код, тестирование и документацию.
- **Авторам DeepSeek** - за секономленные нервы и дни дебаггинга
<img width="960" height="204" alt="image" src="https://github.com/user-attachments/assets/7ad0b4cc-5ef1-4d78-a5ec-4dbe70d37cd6" />

