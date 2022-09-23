#include <Four.h>
#include "imgui.h"
#include "ImGuiConsole.h"

class ExampleLayer : public four::Layer
{
public:
	ExampleLayer()
		:Layer("Example")
	{
	}

	void OnUpdate(four::Deltatime deltaTime) override
	{
		// sample check if key down
		if (four::Input::IsKeyPressed(four::Key::KEY_TAB))
		{
			FOUR_LOG_INFO("Tab Key Down");
		}
	}

	void OnEvent(four::Event& event) override
	{
		
	}

	void OnImGuiRender() override
	{
		bool p_open = true;
		m_console.Draw("Console", &p_open);

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Files"))
			{
				if (ImGui::MenuItem("Close")) { four::Application::Get().Close(); }
				ImGui::Separator();

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}
	}

private:
	ImGuiConsole m_console;
};


class Sandbox : public four::Application
{
public:
	Sandbox()
	{
		PushLayer(new ExampleLayer());
	}
	~Sandbox()
	{

	}
};



four::Application* four::CreateApplication()
{
	return new Sandbox();
}