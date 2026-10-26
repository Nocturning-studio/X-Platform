////////////////////////////////////////////////////////////////////////////////
// Created: 23.10.2026 0:16:30
// Author: NS_Deathman
// File: IGameModule.h
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include "xrFrameworkAPI.h"
#include "GameObjectFactory.h"
#include "ILoadingScreen.h"
////////////////////////////////////////////////////////////////////////////////
class IAppContext;
////////////////////////////////////////////////////////////////////////////////
class ENGINE_API IGameModule
{
  public:
	virtual ~IGameModule() = default;

	// ---- Идентификация ----
	virtual const char* GetName() const = 0;

	// ---- Фабрики (для NEW_INSTANCE / DEL_INSTANCE) ----
	virtual TGameObjectFactoryCreate* GetGameObjectCreate() const = 0;
	virtual TGameObjectFactoryDestroy* GetGameObjectDestroy() const = 0;

	// ---- Регистрация ----
	// Вызывается framework'ом после инициализации соответствующих подсистем.
	// Если подсистема отключена в opts — метод не вызывается.
	virtual void RegisterConsoleCommands() {}
	virtual void RegisterInputCommands() {}

	// ---- Game persistent ----
	virtual IAppContext* CreatePersistent() = 0;
	virtual void DestroyPersistent(IAppContext*) = 0;

	// ---- Loading screen ----
	virtual ILoadingScreen* CreateLoadingScreen() = 0;
	virtual void DestroyLoadingScreen(ILoadingScreen*) = 0;

	virtual void OnFrameworkShutdown() {}
};
////////////////////////////////////////////////////////////////////////////////
