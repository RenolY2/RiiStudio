#pragma once

#include <core/Applet.hpp>
#include <ui/widgets/Dockspace.hpp>
#include <ui/ThemeManager.hpp>


class RiiCore : public Applet
{
public:

	// With a simple applet, the core resource would be the current file itself.
	// However, with the actual editor, the core resource needs to reflect the state of the editor itself and its plugin windows.
	// TODO
	struct EditorCoreRes : public CoreResource
	{
	};


	WindowContext makeWindowContext() override
	{
		return WindowContext(getSelectionManager(), mCoreRes);
	}

	void drawRoot() override;

private:
	DockSpace mDockSpace;
	EditorCoreRes mCoreRes;

	// TODO: Dedicated theme manager
	ThemeManager::BasicTheme mThemeSelection = ThemeManager::BasicTheme::ImDark;
	ThemeManager mThemeManager;
};
