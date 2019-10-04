#pragma once

#include "LibRiiEditor/pluginapi/Plugin.hpp"
#include "LibRiiEditor/ui/widgets/Outliner.hpp"
#include "WindowManager.hpp"

struct EditorWindow : public WindowManager, public Window
{
	~EditorWindow() override = default;
	EditorWindow(const pl::FileState& registration);

	void draw(WindowContext* ctx) noexcept override final;

	const pl::FileState& mEditor;
};
