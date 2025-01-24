// GLEW
#include <GL/glew.h>

// SDL
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

// ImGui
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

// standard
#include <iostream>
#include <sstream>

#include "MyApp.h"

int main( int argc, char* args[] )
{
	//
	// 1. lépés: inicializáljuk az SDL-t
	//

	// Állítsuk be a hiba Logging függvényt.
	SDL_LogSetPriority(SDL_LOG_CATEGORY_ERROR, SDL_LOG_PRIORITY_ERROR);
	// a grafikus alrendszert kapcsoljuk csak be, ha gond van, akkor jelezzük és lépjünk ki
	if ( SDL_Init( SDL_INIT_VIDEO ) == -1 )
	{
		// irjuk ki a hibat es termináljon a program
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[SDL initialization] Error during the SDL initialization: %s", SDL_GetError());
		return 1;
	}

	// Miután az SDL Init lefutott, kilépésnél fusson le az alrendszerek kikapcsolása.
	// Így akkor is lefut, ha valamilyen hiba folytán lépünk ki.
	std::atexit(SDL_Quit);
			
	//
	// 2. lépés: állítsuk be az OpenGL-es igényeinket, hozzuk létre az ablakunkat, indítsuk el az OpenGL-t
	//

	// 2a: OpenGL indításának konfigurálása, ezt az ablak létrehozása előtt kell megtenni!

	// beállíthatjuk azt, hogy pontosan milyen OpenGL context-et szeretnénk létrehozni - ha nem tesszük, akkor
	// automatikusan a legmagasabb elérhető verziójút kapjuk
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifdef _DEBUG 
	// ha debug módú a fordítás, legyen az OpenGL context is debug módban, ekkor működik a debug callback 
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif 

	// állítsuk be, hogy hány biten szeretnénk tárolni a piros, zöld, kék és átlátszatlansági információkat pixelenként
	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE,         32);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,            8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,          8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,           8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,          8);
	// duplapufferelés
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,		1);
	// mélységi puffer hány bites legyen
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,          24);

	// antialiasing - ha kell
	//SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS,  1);
	//SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES,  2);

	// hozzuk létre az ablakunkat
	SDL_Window *win = nullptr;
	win = SDL_CreateWindow( "Hello SDL&OpenGL!",		// az ablak fejléce
							100,						// az ablak bal-felső sarkának kezdeti X koordinátája
							100,						// az ablak bal-felső sarkának kezdeti Y koordinátája
							800,						// ablak szélessége
							600,						// és magassága
							SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);			// megjelenítési tulajdonságok


	// ha nem sikerült létrehozni az ablakot, akkor írjuk ki a hibát, amit kaptunk és lépjünk ki
	if (win == nullptr)
	{
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Window creation] Error during the SDL initialization: %s", SDL_GetError());
		return 1;
	}

	//
	// 3. lépés: hozzunk létre az OpenGL context-et - ezen keresztül fogunk rajzolni
	//

	SDL_GLContext	context = SDL_GL_CreateContext(win);
	if (context == nullptr)
	{
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[OGL context creation] Error during the creation of the OGL context: %s", SDL_GetError());
		return 1;
	}	

	// megjelenítés: várjuk be a vsync-et
	SDL_GL_SetSwapInterval(1);

	// indítsuk el a GLEW-t
	GLenum error = glewInit();
	if ( error != GLEW_OK )
	{
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[GLEW] Error during the initialization of glew.");
		return 1;
	}

	// kérdezzük le az OpenGL verziót
	int glVersion[2] = {-1, -1}; 
	glGetIntegerv(GL_MAJOR_VERSION, &glVersion[0]); 
	glGetIntegerv(GL_MINOR_VERSION, &glVersion[1]); 

	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Running OpenGL %d.%d", glVersion[0], glVersion[1]);

	if ( glVersion[0] == -1 && glVersion[1] == -1 )
	{
		SDL_GL_DeleteContext(context);
		SDL_DestroyWindow( win );

		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[OGL context creation] Error during the inialization of the OGL context! Maybe one of the SDL_GL_SetAttribute(...) calls is erroneous.");
		
		return 1;
	}

	std::stringstream window_title;
	window_title << "OpenGL " << glVersion[0] << "." << glVersion[1];
	SDL_SetWindowTitle(win, window_title.str().c_str());

	//Imgui init
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui::StyleColorsDark();

	ImGui_ImplSDL2_InitForOpenGL(win, context);
	ImGui_ImplOpenGL3_Init();

	//
	// 4. lépés: indítsuk el a fő üzenetfeldolgozó ciklust
	// 
	{
		// véget kell-e érjen a program futása?
		bool quit = false;
		// feldolgozandó üzenet ide kerül
		SDL_Event ev;

		// alkalmazás példánya
		CMyApp app;
		if (!app.Init())
		{
			SDL_GL_DeleteContext(context);
			SDL_DestroyWindow(win);
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[app.Init] Error during the initialization of the application!");
			return 1;
		}

		while (!quit)
		{
			// amíg van feldolgozandó üzenet dolgozzuk fel mindet:
			while ( SDL_PollEvent(&ev) )
			{
				ImGui_ImplSDL2_ProcessEvent(&ev);
				bool is_mouse_captured    = ImGui::GetIO().WantCaptureMouse;    //kell-e az imgui-nak az egér
				bool is_keyboard_captured = ImGui::GetIO().WantCaptureKeyboard;	//kell-e az imgui-nak a billentyűzet

				switch (ev.type)
				{
					case SDL_QUIT:
						quit = true;
						break;
					case SDL_KEYDOWN:
						if ( ev.key.keysym.sym == SDLK_ESCAPE )
							quit = true;

						// ALT + ENTER vált teljes képernyőre, és vissza.
						if ( ( ev.key.keysym.sym == SDLK_RETURN )  // Enter le lett nyomva, ...
							 && ( ev.key.keysym.mod & KMOD_ALT )   // az ALTal együtt, ...
							 && !( ev.key.keysym.mod & ( KMOD_SHIFT | KMOD_CTRL | KMOD_GUI ) ) ) // de más modifier gomb nem lett lenyomva.
						{
							Uint32 FullScreenSwitchFlag = ( SDL_GetWindowFlags( win ) & SDL_WINDOW_FULLSCREEN_DESKTOP ) ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP;
							SDL_SetWindowFullscreen( win, FullScreenSwitchFlag );
						}
						if ( !is_keyboard_captured )
							app.KeyboardDown(ev.key);
						break;
					case SDL_KEYUP:
						if ( !is_keyboard_captured )
							app.KeyboardUp(ev.key);
						break;
					case SDL_MOUSEBUTTONDOWN:
						if ( !is_mouse_captured )
							app.MouseDown(ev.button);
						break;
					case SDL_MOUSEBUTTONUP:
						if ( !is_mouse_captured )
							app.MouseUp(ev.button);
						break;
					case SDL_MOUSEWHEEL:
						if ( !is_mouse_captured )
							app.MouseWheel(ev.wheel);
						break;
					case SDL_MOUSEMOTION:
						if ( !is_mouse_captured )
							app.MouseMove(ev.motion);
						break;
					case SDL_WINDOWEVENT:
						// Néhány platformon (pl. Windows) a SIZE_CHANGED nem hívódik meg az első megjelenéskor.
						// Szerintünk ez bug az SDL könytárban.
						// Ezért ezt az esetet külön lekezeljük, 
						// mivel a MyApp esetlegesen tartalmazhat ablak méret függő beállításokat, pl. a kamera aspect ratioját a perspective() hívásnál.
						if ( ( ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ) || ( ev.window.event == SDL_WINDOWEVENT_SHOWN ) )
						{
							int w, h;
							SDL_GetWindowSize( win, &w, &h );
							app.Resize( w, h );
						}
						break;
					default:
						app.OtherEvent( ev );
				}
			}

			// Számoljuk ki az update-hez szükséges idő mennyiségeket!
			static Uint32 LastTick = SDL_GetTicks(); // statikusan tároljuk, mi volt az előző "tick".
			Uint32 CurrentTick = SDL_GetTicks(); // Mi az aktuális.
			SUpdateInfo updateInfo // Váltsuk át másodpercekre!
			{ 
				static_cast<float>(CurrentTick) / 1000.0f, 
				static_cast<float>(CurrentTick - LastTick) / 1000.0f 
			};
			LastTick = CurrentTick; // Mentsük el utolsóként az aktuális "tick"-et!

			app.Update( updateInfo );
			app.Render();

			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplSDL2_NewFrame(); //Ezután lehet imgui parancsokat hívni, egészen az ImGui::Render()-ig

			ImGui::NewFrame();
			app.RenderGUI();
			ImGui::Render();

			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			SDL_GL_SwapWindow(win);
		}

		// takarítson el maga után az objektumunk
		app.Clean();
	} // így az app destruktora még úgy fut le, hogy él a contextünk => a GPU erőforrásokat befoglaló osztályok destruktorai is itt futnak le

	//
	// 5. lépés: lépjünk ki
	// 

	// ImGui de-init
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow( win );

	return 0;
}
