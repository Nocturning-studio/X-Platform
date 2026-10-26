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
#include "IEditorLevel.h"
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
	virtual IAppContext* CreateGameContext() = 0;
	virtual void DestroyGameContext(IAppContext*) = 0;

	virtual IAppContext* CreateEditorContext() { return nullptr; }
	virtual void DestroyEditorContext(IAppContext* c) { DestroyGameContext(c); }

	// ---- Loading screen ----
	virtual ILoadingScreen* CreateLoadingScreen() = 0;
	virtual void DestroyLoadingScreen(ILoadingScreen*) = 0;

	virtual IEditorLevel* CreateEditorLevel() { return nullptr; }
	virtual void DestroyEditorLevel(IEditorLevel* l) { if (l) { l->net_Stop(); xr_delete(l); } }

	virtual void OnFrameworkShutdown() {}
};
////////////////////////////////////////////////////////////////////////////////
