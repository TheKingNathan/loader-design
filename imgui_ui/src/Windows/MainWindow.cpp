#include "MainWindow.hpp"

#include "../Utils/Utils.hpp"
#include "../Widgets/UtilityWidgets.hpp"

#include <thread>
#include <vector>

namespace MainWindow {
	bool g_once = false;
	bool g_injectionOnce = false;
	bool g_injecting = false;
	bool g_statusWorker = false;
	bool g_doAnim = false;

	float g_animationProgress1 = 0.f;
	float g_animationProgress2 = 0.f;
	float g_animationProgress3 = 0.f;
	float g_animationProgress4 = 0.f;
	int g_moduleIndex = 0;

	std::string g_welcomeMsg;
	std::string g_status;

	std::vector<std::string> g_modules;

	bool ModulesCallback( void* data, int n, const char** out_text ) {
		const std::vector<std::string>* v = ( std::vector<std::string>* )data;
		*out_text = ( *v )[ n ].c_str( );
		return true;
	}

	void StatusWorker( ) {
		while( g_statusWorker ) {
			g_status = std::string( GetStatus( ) );
			std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
		}
	}

	void InjectionWorker( char* mod ) {
		if( Inject( mod, false ) ) {
			g_statusWorker = false;
			g_injecting = false;
			g_status = "Successfully loaded";
			//Utils::Shout( "successfully loaded", "Infinity", true );
		}

		else {
			Utils::Shout( "module failed to pass", "Infinity" );
		}
	}


	void Render( ) {
		auto wSize = Utils::GetWindowSize( );

		if( g_animationProgress1 <= 1.0f ) {
			g_animationProgress1 = ( g_animationProgress1 + static_cast< float >( 0.05f * ( 1.0f - g_animationProgress1 ) ) );
		}

		if( g_animationProgress2 <= 1.0f && g_injecting ) {
			g_animationProgress2 = ( g_animationProgress2 + static_cast< float >( 0.025f * ( 1.0f - g_animationProgress2 ) ) );
		}

		if( g_animationProgress3 <= 0.99999f && ( g_status.find( "loaded" ) != std::string::npos ) ) {
			g_animationProgress3 = ( g_animationProgress3 + static_cast< float >( 0.025f * ( 1.0f - g_animationProgress3 ) ) );
		}

		if( g_animationProgress3 >= 0.99999f ) {
			static int nTickcount = GetTickCount64( );
			if( fabs( GetTickCount64( ) - nTickcount ) > 1000 ) {
				g_animationProgress4 = ( g_animationProgress4 + static_cast< float >( 0.03f * ( 1.0f - g_animationProgress4 ) ) );
			}
		}

		// nasty
		g_animationProgress1 *= ( 1.0f - g_animationProgress2 );

		ImGui::PushStyleVar( ImGuiStyleVar_Alpha, g_animationProgress1 );
		{
			ImGui::GetBackgroundDrawList( )->AddRectFilled( { 0, 0 }, wSize, ImColor( 30, 30, 30, 255 ) );
			ImGui::GetBackgroundDrawList( )->AddRect( { 0, 0 }, wSize, ImColor( 0, 150, 255, 255 ), 0.f, 15, 2.f );

			if( g_pLogoTexture ) {
				// note - alpha;
				// only cos ::AddImage takes min and max, not pos and size
				// this makes life 10000x easier (incase i want to do some epic math shit)
				auto fnImageWrapper = [ & ] ( void* texture, ImVec2 pos, ImVec2 size )->void {
					// nice, no + operator on ImVec2, thx ocornut fkn paster idiot
					ImGui::GetBackgroundDrawList( )->AddImage( texture, pos, ImVec2( pos.x + size.x, pos.y + size.y ),
						ImVec2( 0, 0 ), ImVec2( 1, 1 ), ImColor( 255, 255, 255, int( 255 * g_animationProgress1 ) ) );
				};

				fnImageWrapper( g_pLogoTexture, ImVec2( 100, ( -100 ) + ( 95 * g_animationProgress1 ) ), ImVec2( 200, 120 ) );
			}

			if( !g_once ) {
				g_once = true;
				g_welcomeMsg = "Welcome, " + std::string( GetUsername( ) );
				g_modules = Utils::SplitString( std::string( GetModules( ) ), "##" );
			}

			ImGui::PushFont( g_fontMd );
			auto tSize = ImGui::CalcTextSize( g_welcomeMsg.c_str( ) );
			float tCX = ( wSize.x - tSize.x ) / 2;
			ImGui::GetBackgroundDrawList( )->AddText( { tCX, 115 }, ImColor( 0, 150, 255, int( 255 * g_animationProgress1 ) ), g_welcomeMsg.c_str( ) );
			ImGui::PopFont( );

			if( g_injecting ) {
				if( !g_injectionOnce ) {
					g_injectionOnce = true;
					g_statusWorker = true;
					std::thread( InjectionWorker, ( char* )g_modules[ g_moduleIndex ].c_str( ) ).detach( );
					std::thread( StatusWorker ).detach( );
				}

				float sCX = ( wSize.x - 100 ) / 2;
				ImGui::Spinner( "test", { sCX, 150 - ( 50 * g_animationProgress2 ) }, 50, 2.f, ImColor( 0, 150, 255, int( 255 * g_animationProgress2 ) ) );
			}
			else {
				ImGuiWindowFlags window_flags = 0;
				window_flags |= ImGuiWindowFlags_NoTitleBar;
				window_flags |= ImGuiWindowFlags_NoMove;
				window_flags |= ImGuiWindowFlags_NoResize;
				window_flags |= ImGuiWindowFlags_NoCollapse;
				window_flags |= ImGuiWindowFlags_NoNav;
				window_flags |= ImGuiWindowFlags_NoBackground;

				ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, { 0, 0 } );
				ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, { 8, 8 } );
				ImGui::SetNextWindowPos( { 5, 150 } );
				ImGui::SetNextWindowSize( { wSize.x, 300 } );
				ImGui::Begin( "##main", ( bool* )0, window_flags );

				ImGui::Indent( ( ( wSize.x - ( wSize.x * 0.8 ) ) / 2 ) - 5 );
				float flWidth = wSize.x * 0.8;
				float flHeight = floor( ImGui::GetTextLineHeightWithSpacing( ) * g_modules.size( ) + ImGui::GetStyle( ).FramePadding.y * 2.0f );
				ImGui::SetNextItemWidth( flWidth );

				auto vPos = ImGui::GetCursorScreenPos( );
				vPos.y += 2;
				ImGui::ListBox( "", &g_moduleIndex, ModulesCallback, &g_modules, g_modules.size( ) );

				ImGui::GetBackgroundDrawList( )->AddRect( vPos, ImVec2( vPos.x + ( flWidth ), ( vPos.y + flHeight ) - 8 ), ImColor( 60, 60, 60, int( 255 * g_animationProgress1 ) ), 0.f, 15, 1.f );

				ImGui::InvisibleButton( "##invs1", { 1.f, 8.f } );

				ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, { 8, 4 } );
				auto vPos2 = ImGui::GetCursorScreenPos( );
				if( ImGui::Button( "Load", { flWidth, 25 } ) ) {
					if( g_moduleIndex > -1 ) {
						g_injectionOnce = false;
						g_injecting = true;
					}
				}

				ImGui::GetBackgroundDrawList( )->AddRect( ImVec2( vPos2.x - 1, vPos2.y - 1 ), ImVec2( vPos2.x + ( flWidth + 1 ), vPos2.y + 26 ), ImColor( 60, 60, 60, int( 255 * g_animationProgress1 ) ), 0.f, 15, 1.f );

				ImGui::End( );
				ImGui::PopStyleVar( 3 );
			}


			ImGui::PushFont( g_fontMd );
			tSize = ImGui::CalcTextSize( g_status.c_str( ) );
			tCX = ( wSize.x - tSize.x ) / 2;
			ImGui::GetBackgroundDrawList( )->AddText( { tCX, ( 100 - ( ( 50 * g_animationProgress2 ) - ( 80 * ( g_animationProgress3 ) ) ) ) + ( 300 * g_animationProgress4 ) },
				ImColor( 192, 192, 192, int( ( 255 * g_animationProgress2 ) * ( 1.0f - g_animationProgress4 ) ) ), g_status.c_str( ) );

			if( g_animationProgress4 >= 0.95f ) {
				exit( 1 );
			}

			ImGui::PopFont( );
		}
		ImGui::PopStyleVar( );
	}
}