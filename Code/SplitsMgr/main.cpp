//------------------------------------------------------------------------
//Author : Philippe OFFERMANN
//Date : 17/06/2025
//Description : Entry point of the program
//------------------------------------------------------------------------

#include <FZN/Includes.h>
#include <FZN/Managers/DataManager.h>
#include <FZN/Managers/WindowManager.h>

#include <SFML/Graphics/RenderWindow.hpp>

#include "SplitsManagerApp.h"


int __stdcall WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	fzn::FazonCore::CreateInstance( { "1Y1G Splits Manager", FZNProjectType::Application } );

	//Changing the titles of the window and the console
	g_pFZN_Core->ConsoleTitle( g_pFZN_Core->GetProjectName().c_str() );
	//g_pFZN_Core->HideConsole();

	g_pFZN_Core->GreetingMessage();
	g_pFZN_Core->SetConsolePosition( sf::Vector2i( 10, 10 ) );

	//Loading of the resources that don't belong in a resource group and filling of the map containing the paths to the resources)
	g_pFZN_DataMgr->LoadResourceFile( DATAPATH( "XMLFiles/Resources" ) );

	g_pFZN_WindowMgr->AddWindow( 900, 800, sf::Style::Close | sf::Style::Resize, g_pFZN_Core->GetProjectName().c_str() );
	g_pFZN_WindowMgr->SetWindowFramerate(60);
	g_pFZN_WindowMgr->SetWindowClearColor( sf::Color::Black );
	
	g_pFZN_WindowMgr->SetIcon( DATAPATH( "Misc/fzn.png" ) );

	auto app = SplitsMgr::SplitsManagerApp{};

	//Game loop (add callbacks to your functions so they can be called in there)
	g_pFZN_Core->GameLoop();
}
