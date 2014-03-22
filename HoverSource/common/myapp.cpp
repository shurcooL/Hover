
#include "common.h"
#include <conio.h> // kbhit()

FLOAT VP_FOV =		( 0.38f * g_PI );

//-----------------------------------------------------------------------------
// Internal function prototypes and variables
//-----------------------------------------------------------------------------
enum APPMSGTYPE { MSG_NONE, MSGERR_APPMUSTEXIT, MSGWARN_SWITCHEDTOSOFTWARE };

static INT     CALLBACK AboutProc( HWND, UINT, WPARAM, LPARAM );
static LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM );

CMyApplication* g_pMyApp;

extern HANDLE g_SoundNotificationEvents[1];


CMyApplication::CMyApplication(){
	
	m_pFramework   = NULL;
	m_hWnd         = NULL;
	m_pDD          = NULL;
	m_pD3D         = NULL;
	m_pd3dDevice   = NULL;
	
	m_pddsRenderTarget     = NULL;
	m_pddsRenderTargetLeft = NULL;
	
	m_bActive         = FALSE;
	m_bReady          = FALSE;
	m_bAttemptingInitialization = FALSE;
	m_bFrameMoving    = TRUE;
	m_bSingleStep     = FALSE;
	
	m_bAppUseZBuffer	= TRUE;
	m_bAppUseStereo 	= FALSE;
	m_bShowStats			= TRUE;
	m_fnConfirmDevice = NULL;

	m_bVSync = TRUE;

	playerRacerNum = 0;
	bPlayerPitchAssist = FALSE; //TRUE;
    bAutopilot = FALSE;
	playerPitchAssistLevel = 1.0f;

	//bTestShadow = TRUE;
	bTestShadow = TRUE;
	bRenderShadowTexture = FALSE;
    bDisplayShadowTexture = FALSE;
	shadowTexture.pddsSurface = NULL;

    bWireframeTerrain = FALSE;
    bDisplayLiftLines = FALSE;

	elapsedTime = 0;

	cam.offsetNum = 1;

	g_pMyApp = this;


	track.triGroups = NULL;

	for(DWORD i=0; i < MAX_TERRAIN_TYPES; i++){

		track.terrTypeIndices[i] = NULL;
		track.terrTypeBlendIndices[i];
		track.terrTypeBlendOnIndices[i];
	}

	track.terrTypeRuns = NULL;
	track.terrTypeNodes = NULL;

	track.terrVB = NULL;
	track.terrTransformedVB = NULL;

	track.terrCoords = NULL;

	track.navCoords = NULL;
	track.navCoordLookupRuns = NULL;
	track.navCoordLookupNodes = NULL;

	for(i=0; i < MAX_RACERS; i++){
		track.racerStartPositions[i] = D3DVECTOR( 50 * TERR_SCALE, 100, 10 * TERR_SCALE );
	}
}




//-----------------------------------------------------------------------------
// Name: Create()
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CMyApplication::Create( HINSTANCE hInst, TCHAR* strCmdLine )
{
    HRESULT hr;

		// OutputToLog( "\n\nHover Log File\n\n" );

		// OutputToLog( "Enumerating devices..." );

    // Enumerate available D3D devices. The callback is used so the app can
    // confirm/reject each enumerated device depending on its capabilities.
    if( FAILED( hr = D3DEnum_EnumerateDevices( m_fnConfirmDevice ) ) )
    {
        //DisplayFrameworkError( hr, MSGERR_APPMUSTEXIT );
			if( hr == D3DENUMERR_SUGGESTREFRAST ){
				MyAlert( NULL, "No acceptible Direct3D devices were found" );
			}
			else{
				MyAlert( NULL, "No Direct3D devices were found" );
			}
      return hr;
    }

		// OutputToLog( "Done\n" );

		// OutputToLog( "Selecting a default device..." );

    // Select a device. Pick hardware, or hardware T&L, or software.
    if( FAILED( hr = MyEnum_SelectDefaultDevice( &m_pDeviceInfo, 0 ) ) ) // D3DENUM_SOFTWAREONLY ) ) )
    {
 				MyAlert( NULL, "Could not select a Direct3D device" );
        return hr;
    }

		CHAR msg[256];
		sprintf( msg, "Done (%s)\n", m_pDeviceInfo->strDesc );
		// OutputToLog( msg );

		//D3DEnum_UserChangeDevice( &m_pDeviceInfo );

    // Create a new CD3DFramework class. This class does all of our D3D
    // initialization and manages the common D3D objects.
    if( NULL == ( m_pFramework = new CMyFramework7() ) )
    {
 				MyAlert( NULL, "Out of memory" );
//        DisplayFrameworkError( E_OUTOFMEMORY, MSGERR_APPMUSTEXIT );
        return E_OUTOFMEMORY;
    }

		// OutputToLog( "Creating window..." );

    // Register the window class
    WNDCLASS wndClass = { 0, WndProc, 0, 0, hInst,
                          LoadIcon( hInst, MAKEINTRESOURCE(IDI_MAIN_ICON) ),
                          LoadCursor( NULL, IDC_ARROW ), 
                          (HBRUSH)GetStockObject(WHITE_BRUSH),
                          NULL, _T("D3D Window") };
    RegisterClass( &wndClass );

    // Create the render window
    m_hWnd = CreateWindow( _T("D3D Window"), m_strWindowTitle,
                           WS_OVERLAPPEDWINDOW | WS_VISIBLE,
 //                          WS_POPUP | WS_VISIBLE,
                           CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0L,
                           NULL, 
                           hInst, 0L );
		// OutputToLog( "Done\n" );

    // Initialize the app's custom scene stuff
    if( FAILED( hr = Init() ) )
    {
         return hr;
    }

		m_bAttemptingInitialization = TRUE;

    // Initialize the 3D environment for the app
    if( SUCCEEDED( hr = Initialize3DEnvironment() ) ){
	    m_bReady = TRUE;
			gameState = GAMESTATE_PLAYERRACE;
			transitionState = TRANSITIONSTATE_FADEIN;
			transitionTime = FADEIN_DURATION;
			RestartRace();
			hr = Update();
		}
		else{ // if failed because of shadow test, then retry (w/ low-q shadows)
			bTestShadow = FALSE;
			bRenderShadowTexture = FALSE;
			if( SUCCEEDED( hr = Change3DEnvironment() ) ){
				m_bReady = TRUE;
				gameState = GAMESTATE_PLAYERRACE;
				transitionState = TRANSITIONSTATE_FADEIN;
				transitionTime = FADEIN_DURATION;
				RestartRace();
				hr = Update();
			}
		}

		while( FAILED( hr ) && SUCCEEDED( ForcedVideoOptions() ) ){
			m_bReady = FALSE;
			if( SUCCEEDED( hr = Change3DEnvironment() ) ){
				m_bReady = TRUE;
				gameState = GAMESTATE_PLAYERRACE;
				transitionState = TRANSITIONSTATE_FADEIN;
				transitionTime = FADEIN_DURATION;
				RestartRace();
				hr = Update();
			}
    }
 
		if( FAILED( hr ) ){
			return E_FAIL;
		}

		m_bAttemptingInitialization = FALSE;

		gameState = GAMESTATE_DEMORACE;
		transitionState = TRANSITIONSTATE_FADEIN;
		transitionTime = FADEIN_DURATION + 0.0f;
		RestartRace();

    m_pd3dDevice->SetRenderState( D3DRENDERSTATE_FOGENABLE, 	 TRUE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTFG_LINEAR ); 
	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTFG_LINEAR ); 

    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: Run()
// Desc: Message-processing loop. Idle time is used to render the scene.
//-----------------------------------------------------------------------------
INT CMyApplication::Run(){

	HRESULT hr;

	// Load keyboard accelerators
	HACCEL hAccel = LoadAccelerators( NULL, MAKEINTRESOURCE(IDR_MAIN_ACCEL) );
	
	// Now we're ready to recieve and process Windows messages.
	BOOL bGotMsg;
	MSG  msg;
	DWORD dwResult;
	PeekMessage( &msg, NULL, 0U, 0U, PM_NOREMOVE );
	
	while( WM_QUIT != msg.message  ){
		

////// SOUND ////////////////////////
#ifdef APP_GAME
		dwResult = MsgWaitForMultipleObjects( 1, g_SoundNotificationEvents, 
			FALSE, 0, QS_ALLINPUT );

		if( dwResult == WAIT_OBJECT_0 ){

			if( FAILED( hr = HandleNotification( FALSE ) ) ){

				MyAlert( &hr, "Failed handling DirectSound notification" );
				DestroyWindow( m_hWnd );
			}
		}
#endif
/////////////////////////////////////////////////////////////


	//	else if( dwResult == WAIT_OBJECT_0 + 1 ){
			
			// Use PeekMessage() if the app is active, so we can use idle time to
			// render the scene. Else, use GetMessage() to avoid eating CPU time.
			if( m_bActive )
				bGotMsg = PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE );
			else
				bGotMsg = GetMessage( &msg, NULL, 0U, 0U );
			
			if( bGotMsg )
			{
				// Translate and dispatch the message
				if( 0 == TranslateAccelerator( m_hWnd, hAccel, &msg ) )
				{
					TranslateMessage( &msg );
					DispatchMessage( &msg );
				}
			}
			else
			{
				// do stuff during idle time (no messages are waiting)
				if( m_bActive && m_bReady )
				{
					if( FAILED( Update() ) )
						DestroyWindow( m_hWnd );
				}
			}
		//}
	}
		
	return msg.wParam;
}




//-----------------------------------------------------------------------------
// Name: WndProc()
// Desc: Static msg handler which passes messages to the application class.
//-----------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    if( g_pMyApp )
        return g_pMyApp->MsgProc( hWnd, uMsg, wParam, lParam );
//        return g_pMyApp->MyMsgProc( hWnd, uMsg, wParam, lParam );

    return DefWindowProc( hWnd, uMsg, wParam, lParam );
}


//-----------------------------------------------------------------------------
// Name: DisplayFrameworkError()
// Desc: Displays error messages in a message box
//-----------------------------------------------------------------------------
/*
VOID CMyApplication::DisplayFrameworkError( HRESULT hr, DWORD dwType )
{
    TCHAR strMsg[512];

    switch( hr )
    {
        case D3DENUMERR_NODIRECTDRAW:
            lstrcpy( strMsg, _T("Could not create DirectDraw!") );
            break;
        case D3DENUMERR_NOCOMPATIBLEDEVICES:
            lstrcpy( strMsg, _T("Could not find any compatible Direct3D\n"
                     "devices.") );
            break;
        case D3DENUMERR_SUGGESTREFRAST:
            lstrcpy( strMsg, _T("Could not find any compatible devices.\n\n"
                     "Try enabling the reference rasterizer using\n"
                     "EnableRefRast.reg.") );
            break;
        case D3DENUMERR_ENUMERATIONFAILED:
            lstrcpy( strMsg, _T("Enumeration failed. Your system may be in an\n"
                     "unstable state and need to be rebooted") );
            break;
        case D3DFWERR_INITIALIZATIONFAILED:
            lstrcpy( strMsg, _T("Generic initialization error.\n\nEnable "
                     "debug output for detailed information.") );
            break;
        case D3DFWERR_NODIRECTDRAW:
            lstrcpy( strMsg, _T("No DirectDraw") );
            break;
        case D3DFWERR_NODIRECT3D:
            lstrcpy( strMsg, _T("No Direct3D") );
            break;
        case D3DFWERR_INVALIDMODE:
            lstrcpy( strMsg, _T("This sample requires a 16-bit (or higher) "
                                "display mode\nto run in a window.\n\nPlease "
                                "switch your desktop settings accordingly.") );
            break;
        case D3DFWERR_COULDNTSETCOOPLEVEL:
            lstrcpy( strMsg, _T("Could not set Cooperative Level") );
            break;
        case D3DFWERR_NO3DDEVICE:
            lstrcpy( strMsg, _T("Could not create the Direct3DDevice object.") );
            
            if( MSGWARN_SWITCHEDTOSOFTWARE == dwType )
                lstrcat( strMsg, _T("\nThe 3D hardware chipset may not support"
                                    "\nrendering in the current display mode.") );
            break;
        case D3DFWERR_NOZBUFFER:
            lstrcpy( strMsg, _T("No ZBuffer") );
            break;
        case D3DFWERR_INVALIDZBUFFERDEPTH:
            lstrcpy( strMsg, _T("Invalid Z-buffer depth. Try switching modes\n"
                     "from 16- to 32-bit (or vice versa)") );
            break;
        case D3DFWERR_NOVIEWPORT:
            lstrcpy( strMsg, _T("No Viewport") );
            break;
        case D3DFWERR_NOPRIMARY:
            lstrcpy( strMsg, _T("No primary") );
            break;
        case D3DFWERR_NOCLIPPER:
            lstrcpy( strMsg, _T("No Clipper") );
            break;
        case D3DFWERR_BADDISPLAYMODE:
            lstrcpy( strMsg, _T("Bad display mode") );
            break;
        case D3DFWERR_NOBACKBUFFER:
            lstrcpy( strMsg, _T("No backbuffer") );
            break;
        case D3DFWERR_NONZEROREFCOUNT:
            lstrcpy( strMsg, _T("A DDraw object has a non-zero reference\n"
                     "count (meaning it was not properly cleaned up)." ) );
            break;
        case D3DFWERR_NORENDERTARGET:
            lstrcpy( strMsg, _T("No render target") );
            break;
        case E_OUTOFMEMORY:
            lstrcpy( strMsg, _T("Not enough memory!") );
            break;
        case DDERR_OUTOFVIDEOMEMORY:
            lstrcpy( strMsg, _T("There was insufficient video memory "
                     "to use the\nhardware device.") );
            break;
        default:
            lstrcpy( strMsg, _T("Generic application error.\n\nEnable "
                     "debug output for detailed information.") );
    }

    if( MSGERR_APPMUSTEXIT == dwType )
    {
        lstrcat( strMsg, _T("\n\nThis sample will now exit.") );
        MessageBox( NULL, strMsg, m_strWindowTitle, MB_ICONERROR|MB_OK );
    }
    else
    {
        if( MSGWARN_SWITCHEDTOSOFTWARE == dwType )
            lstrcat( strMsg, _T("\n\nSwitching to software rasterizer.") );
        MessageBox( NULL, strMsg, m_strWindowTitle, MB_ICONWARNING|MB_OK );
    }
}
*/



//-----------------------------------------------------------------------------
// Name: Render3DEnvironment()
// Desc: Draws the scene.
//-----------------------------------------------------------------------------
HRESULT CMyApplication::Update()
{
	HRESULT hr;
	
	// Check the cooperative level before rendering
	if( FAILED( hr = m_pDD->TestCooperativeLevel() ) )
	{
		switch( hr )
		{
		case DDERR_EXCLUSIVEMODEALREADYSET:
		case DDERR_NOEXCLUSIVEMODE:
			// Do nothing because some other app has exclusive mode
			return S_OK;
			
		case DDERR_WRONGMODE:
			// The display mode changed on us. Resize accordingly
			if( m_pDeviceInfo->bWindowed )
				return Change3DEnvironment();
			break;
		}
		return hr;
	}

  if( m_bFrameMoving || m_bSingleStep ){
		if( FAILED( UpdateGameState() ) ){
			return E_FAIL;
		}

		if( FAILED( hr = Move() ) ){
			return hr;
		}
		m_bSingleStep = FALSE;
	}


	// Render the scene as normal
	if( FAILED( hr = Render() ) ){
		return hr;
	}

	//m_misc = Magnitude( racers[cam.racerNum].vel );

	//m_misc = racers[0].targetNavCoord;

	//m_misc = racers[playerRacerNum].distToStartCoord;

	ShowStats();
	
	// Show the frame on the primary surface.
	if( FAILED( hr = m_pFramework->ShowFrame( m_bVSync ) ) )
	{
		if( DDERR_SURFACELOST != hr ){
			MyAlert( &hr, "Unable to display frame" );
			return hr;
		}
		
		m_pFramework->RestoreSurfaces();
		RestoreSurfaces();
	}
	
	return S_OK;
}





HRESULT CMyApplication::Render()
{
	HRESULT hr;

	// Clear the viewport
	if( FAILED( hr = m_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,
		track.bgColor, 1.0f, 0L ) ) ){
		MyAlert(&hr, "Unable to clear render surface" );
		return E_FAIL;
	}

	// Begin the scene 
	if( SUCCEEDED( m_pd3dDevice->BeginScene() ) ){

		// if( m_bAttemptingInitialization )
			// OutputToLog( "Rendering terrain..." );

		if( FAILED( RenderTerrain() ) ){
			m_pd3dDevice->EndScene();
			return E_FAIL;
		}

		// if( m_bAttemptingInitialization )
			// OutputToLog( "Done\n" );

		// if( m_bAttemptingInitialization )
			// OutputToLog( "Rendering racers..." );


		if( FAILED( RenderRacers() ) ){
		m_pd3dDevice->EndScene();
			return E_FAIL;
		}

		// if( m_bAttemptingInitialization )
			// OutputToLog( "Done\n" );

		// if( m_bAttemptingInitialization )
			// OutputToLog( "Displaying stats..." );

#ifdef APP_GAME
		if( FAILED( hr = DisplayStats() ) ){
			m_pd3dDevice->EndScene();
			return hr;
		}
#endif
		// if( m_bAttemptingInitialization )
			// OutputToLog( "Done\n" );

		// End the scene.
		m_pd3dDevice->EndScene();

		// bIsFirstRender = FALSE;
	}

	return S_OK;
}





//-----------------------------------------------------------------------------
// Name: Cleanup3DEnvironment()
// Desc: Cleanup scene objects
//-----------------------------------------------------------------------------
VOID CMyApplication::Cleanup3DEnvironment()
{
    m_bActive = FALSE;
    m_bReady  = FALSE;

    if( m_pFramework )
    {
        DeleteDeviceObjects();
        SAFE_DELETE( m_pFramework );

        FinalCleanup();
    }

    D3DEnum_FreeResources();
}



//-----------------------------------------------------------------------------
// Name: Initialize3DEnvironment()
// Desc: Initializes the sample framework, then calls the app-specific function
//       to initialize device specific objects. This code is structured to
//       handled any errors that may occur duing initialization
//-----------------------------------------------------------------------------
HRESULT CMyApplication::Initialize3DEnvironment()
{
    HRESULT hr;
    DWORD   dwFrameworkFlags = 0L;
    dwFrameworkFlags |= ( !m_pDeviceInfo->bWindowed ? D3DFW_FULLSCREEN : 0L );
    dwFrameworkFlags |= (  m_pDeviceInfo->bStereo   ? D3DFW_STEREO     : 0L );
    dwFrameworkFlags |= (  m_bAppUseZBuffer         ? D3DFW_ZBUFFER    : 0L );

		// OutputToLog( "Initializing D3D framework..." );

    // Initialize the D3D framework
    if( SUCCEEDED( hr = m_pFramework->Initialize( m_hWnd,
                     m_pDeviceInfo->pDriverGUID, m_pDeviceInfo->pDeviceGUID,
                     &m_pDeviceInfo->ddsdFullscreenMode, dwFrameworkFlags ) ) )
    {

				// OutputToLog( "Done\n" );

        m_pDD        = m_pFramework->GetDirectDraw();
        m_pD3D       = m_pFramework->GetDirect3D();
        m_pd3dDevice = m_pFramework->GetD3DDevice();

        m_pddsRenderTarget     = m_pFramework->GetRenderSurface();
        m_pddsRenderTargetLeft = m_pFramework->GetRenderSurfaceLeft();

        m_ddsdRenderTarget.dwSize = sizeof(m_ddsdRenderTarget);
        m_pddsRenderTarget->GetSurfaceDesc( &m_ddsdRenderTarget );

        // Let the app run its startup code which creates the 3d scene.
        if( SUCCEEDED( hr = InitDeviceObjects() ) ){
		      return S_OK;
				}

				// OutputToLog( "Failed\n" );
					
        DeleteDeviceObjects();
        m_pFramework->DestroyObjects();
		}
		else{
			// OutputToLog( "Failed\n" );
		}

/*
    // If we get here, the first initialization passed failed. If that was with a
    // hardware device, try again using a software rasterizer instead.
    if( m_pDeviceInfo->bHardware )
    {
        // Try again with a software rasterizer
        DisplayFrameworkError( hr, MSGWARN_SWITCHEDTOSOFTWARE );
        D3DEnum_SelectDefaultDevice( &m_pDeviceInfo, D3DENUM_SOFTWAREONLY );
        //D3DEnum_SelectFullscreenDevice( &m_pDeviceInfo, D3DENUM_SOFTWAREONLY );
        return Initialize3DEnvironment();
    }
*/
    return hr;
}



//-----------------------------------------------------------------------------
// Name: MsgProc()
// Desc: Message handling function.
//-----------------------------------------------------------------------------
LRESULT CMyApplication::MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam,
															 LPARAM lParam )
{
	HRESULT hr;
	
	switch( uMsg )
	{

		case WM_KEYDOWN:
			if( HandleKeyInputEvent( wParam ) ){
				return 0;
			}
			return 1;
/*
		case WM_CHAR:
		
			if( HandleKeyInputEvent( wParam ) ){
				return 0;
			}
			return 1;
*/		
		case WM_PAINT:
			// Handle paint messages when the app is not ready
			if( m_pFramework && !m_bReady )
			{
				if( m_pDeviceInfo->bWindowed )
					m_pFramework->ShowFrame( m_bVSync );
				else
					m_pFramework->FlipToGDISurface( TRUE );
			}
			break;
			
		case WM_MOVE:
			// If in windowed mode, move the Framework's window
			if( m_pFramework && m_bActive && m_bReady && m_pDeviceInfo->bWindowed )
				m_pFramework->Move( (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam) );
			break;
			
		case WM_SIZE:
			// Check to see if we are losing our window...
			if( SIZE_MAXHIDE==wParam || SIZE_MINIMIZED==wParam ){
				m_bActive = FALSE;
			}
			else{
				if( m_bFrameMoving ){
					startTime = timeGetTime() * 0.001f - elapsedTime;
				}
				m_bActive = TRUE;
			}

      // Set exclusive mode access to the mouse based on active state
			SetAcquire( m_bActive );

			// A new window size will require a new backbuffer
			// size, so the 3D structures must be changed accordingly.
			if( m_bActive && m_bReady && m_pDeviceInfo->bWindowed )
			{
				Pause(TRUE);

                #ifdef APP_GAME
				    StopAllSoundInstances();
                #endif
				
				if( FAILED( hr = Change3DEnvironment() ) ){
					SendMessage( hWnd, WM_CLOSE, 0, 0 );
					return 0;
				}
				
				Pause(FALSE);
			}
			break;
			
		case WM_SETCURSOR:
			// Prevent a cursor in fullscreen mode
			if( m_bActive && m_bReady && !m_pDeviceInfo->bWindowed )
			{
				SetCursor(NULL);
				return 1;
			}
			break;
			
		case WM_ENTERMENULOOP:
			// Pause the app when menus are displayed
			Pause(TRUE);
			break;
		case WM_EXITMENULOOP:
			Pause(FALSE);
			break;
			
		case WM_ENTERSIZEMOVE:
			// Halt frame movement while the app is sizing or moving
			break;
		case WM_EXITSIZEMOVE:
			if( m_bFrameMoving ){
        startTime = timeGetTime() * 0.001f - elapsedTime;
			}
			break;
/*			
		case WM_CONTEXTMENU:
			// Handle the app's context menu (via right mouse click) 
			TrackPopupMenuEx(
				GetSubMenu( LoadMenu( 0, MAKEINTRESOURCE(IDR_POPUP) ), 0 ),
				TPM_VERTICAL, LOWORD(lParam), HIWORD(lParam), hWnd, NULL );
			break;
*/			
		case WM_NCHITTEST:
			// Prevent the user from selecting the menu in fullscreen mode
			if( !m_pDeviceInfo->bWindowed )
				return HTCLIENT;
			
			break;
			
		case WM_POWERBROADCAST:
			switch( wParam )
			{
			case PBT_APMQUERYSUSPEND:
				// At this point, the app should save any data for open
				// network connections, files, etc.., and prepare to go into
				// a suspended mode.
				return OnQuerySuspend( (DWORD)lParam );
				
			case PBT_APMRESUMESUSPEND:
				// At this point, the app should recover any data, network
				// connections, files, etc.., and resume running from when
				// the app was suspended.
				return OnResumeSuspend( (DWORD)lParam );
			}
			break;
			
			case WM_SYSCOMMAND:
				// Prevent moving/sizing and power loss in fullscreen mode
				switch( wParam )
				{
				case SC_MOVE:
				case SC_SIZE:
				case SC_MAXIMIZE:
				case SC_MONITORPOWER:
					if( FALSE == m_pDeviceInfo->bWindowed )
						return 1;
					break;
				}
				break;
				
			case WM_COMMAND:
				switch( LOWORD(wParam) )
				{
/*
				case IDM_TOGGLESTART:
					// Toggle frame movement
					m_bFrameMoving = !m_bFrameMoving;
						
					if( m_bFrameMoving ){
						startTime += timeGetTime() * 0.001f - pauseTime;
						pauseTime = timeGetTime() * 0.001f;
					}
					else
						pauseTime = timeGetTime() * 0.001f;
					break;
						
				case IDM_SINGLESTEP:
					// Single-step frame movement
					if( FALSE == m_bFrameMoving )
						startTime += timeGetTime() * 0.001f - ( pauseTime + 0.1f );
						
					pauseTime	 = timeGetTime() * 0.001f;
					m_bFrameMoving = FALSE;
					m_bSingleStep  = TRUE;
					break;
*/						
				case IDM_CHANGEDEVICE:
					// Display the device-selection dialog box.
					if( m_bActive && m_bReady )
					{
						Pause(TRUE);

                        #ifdef APP_GAME
						    StopAllSoundInstances();

						if( IsStreamingBufferPlaying() ){
							StopBuffer();
						}
                        #endif
							
						if( SUCCEEDED( VideoOptions() ) )
						{
							hr = Change3DEnvironment();

							while( FAILED( hr ) && SUCCEEDED( ForcedVideoOptions() ) ){
								hr = Change3DEnvironment();
							}

							if( FAILED( hr ) ){
								SendMessage( hWnd, WM_CLOSE, 0, 0 );
								return 0;
							}
						}

						Pause(FALSE);
					}
					return 0;
						
				case IDM_TOGGLEFULLSCREEN:
					// Toggle the fullscreen/window mode
					if( m_bActive && m_bReady )
					{
						Pause(TRUE);

						m_pDeviceInfo->bWindowed = !m_pDeviceInfo->bWindowed;
							
						if( FAILED( hr = Change3DEnvironment() ) ){
							SendMessage( hWnd, WM_CLOSE, 0, 0 );
							return 0;
						}
							
						Pause(FALSE);
					}
					return 0;
/*						
				case IDM_ABOUT:
					// Display the About box
					Pause(TRUE);
					DialogBox( (HINSTANCE)GetWindowLong( hWnd, GWL_HINSTANCE ),
						MAKEINTRESOURCE(IDD_ABOUT), hWnd, AboutProc );
					Pause(FALSE);
					return 0;
*/						
				case IDM_EXIT:
					// Recieved key/menu command to exit app
					SendMessage( hWnd, WM_CLOSE, 0, 0 );
					return 0;
				}
				break;
					
			case WM_GETMINMAXINFO:
					((MINMAXINFO*)lParam)->ptMinTrackSize.x = 100;
					((MINMAXINFO*)lParam)->ptMinTrackSize.y = 100;
					break;
						
			case WM_CLOSE:
					DestroyWindow( hWnd );
					return 0;
						
			case WM_DESTROY:
					Cleanup3DEnvironment();
					PostQuitMessage(0);
					return 0;
		}
		
		return DefWindowProc( hWnd, uMsg, wParam, lParam );
}


//-----------------------------------------------------------------------------
// Name: AboutProc()
// Desc: Minimal message proc function for the about box
//-----------------------------------------------------------------------------
BOOL CALLBACK AboutProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM )
{
    if( WM_COMMAND == uMsg )
        if( IDOK == LOWORD(wParam) || IDCANCEL == LOWORD(wParam) )
            EndDialog( hWnd, TRUE );

    return WM_INITDIALOG == uMsg ? TRUE : FALSE;
}


//-----------------------------------------------------------------------------
// Name: Pause()
// Desc: Called in to toggle the pause state of the app. This function
//       brings the GDI surface to the front of the display, so drawing
//       output like message boxes and menus may be displayed.
//-----------------------------------------------------------------------------
VOID CMyApplication::Pause( BOOL bPause )
{
    static INT32 dwAppPausedCount = 0;

    dwAppPausedCount += ( bPause ? +1 : -1 );
		if( dwAppPausedCount < 0 ){
			dwAppPausedCount = 0;
		}
    m_bReady          = ( dwAppPausedCount ? FALSE : TRUE );

    // Handle the first pause request (of many, nestable pause requests)
    if( bPause && ( 1 == dwAppPausedCount ) )
    {
        // Get a surface for the GDI
        if( m_pFramework )
            m_pFramework->FlipToGDISurface( TRUE );

        // Stop the scene from animating
//        if( m_bFrameMoving )
//           pauseTime = timeGetTime() * 0.001f;
    }

    if( 0 == dwAppPausedCount )
    {
        // Restart the scene
			if( m_bFrameMoving ){
        startTime = timeGetTime() * 0.001f - elapsedTime;
			}
    }
}



//-----------------------------------------------------------------------------
// Name: ShowStats()
// Desc: Shows frame rate and dimensions of the rendering device.
//-----------------------------------------------------------------------------
VOID CMyApplication::ShowStats()
{
    static FLOAT fFPS      = 0.0f;
    static FLOAT fLastTime = 0.0f;
    static DWORD dwFrames  = 0L;

    // Keep track of the time lapse and frame count
    FLOAT fTime = timeGetTime() * 0.001f; // Get current time in seconds
    ++dwFrames;

    // Update the frame rate once per second
    if( fTime - fLastTime > 1.0f )
    {
        fFPS      = dwFrames / (fTime - fLastTime);
        fLastTime = fTime;
        dwFrames  = 0L;
    }

    // Setup the text buffer to write out dimensions
    TCHAR buffer[80];

		// if( gameState == GAMESTATE_DEMORACE ){
		// 	OutputText( m_vp.dwWidth / 2 - 125, 8, "See readme.txt for playing instructions" );
		// }

		// OutputText( 0, m_vp.dwHeight - 20, " Hover © 2000 Eric Undersander" );
//OutputText( 0, m_vp.dwHeight - 18, " <eundersan@mail.utexas.edu>" );

    sprintf( buffer, "%7.02f fps (%dx%dx%d)", fFPS,
             m_ddsdRenderTarget.dwWidth, m_ddsdRenderTarget.dwHeight, 
             m_ddsdRenderTarget.ddpfPixelFormat.dwRGBBitCount );
		OutputText( 0, m_vp.dwHeight - 20, buffer );

#ifdef APP_EDITOR
        extern FLOAT terrTypeSlopeThresholds[3];
        extern LONG LOD_SETTING;

		sprintf( buffer, "  Slope threshhold: %7.02f", terrTypeSlopeThresholds[1] );
		OutputText( 0, m_vp.dwHeight - 40, buffer );
#endif
    
//		sprintf( buffer, "(%d,%d)",
//			track.renderedTerrCoords.endX - track.renderedTerrCoords.startX,
//			track.renderedTerrCoords.endZ - track.renderedTerrCoords.startZ	);
//		OutputText( 0, m_vp.dwHeight - 46, buffer );
/*
		CHAR gameStateDescs[][32] = { "demo", "player", "post" };
		sprintf( buffer, "gameState: %s", gameStateDescs[gameState] );
		OutputText( 0, m_vp.dwHeight - 60, buffer );

		CHAR transStateDescs[][32] = { "fade-in", "normal", "fade-out" };
		sprintf( buffer, "transState: %s (%7.02f)", transStateDescs[transitionState], transitionTime );
		OutputText( 0, m_vp.dwHeight - 74, buffer );
*/

    static FLOAT trisPerFrame= 0.0f;
    static DWORD frames  = 0;
	static DWORD tris = 0;

	static DWORD oldCamOffsetNum = 0;

	if( oldCamOffsetNum != cam.offsetNum ){
		oldCamOffsetNum = cam.offsetNum;
		trisPerFrame= 0.0f;
		frames  = 0;
		tris = 0;
	}

// Keep track of the time lapse and frame count
    frames++;
	trisPerFrame = m_misc; // ( trisPerFrame * ( frames - 1 ) + (FLOAT)m_misc ) / (FLOAT)frames;

	sprintf( buffer, "  Tri count: %7.02f", trisPerFrame );
	OutputText( 0, m_vp.dwHeight - 60, buffer );

	static DWORD maxVertices = 0;
	if( m_misc > maxVertices ){ maxVertices = m_misc; }

	//m_misc = Magnitude( racers[playerRacerNum].vel ) / RACER_REAL_MAXVEL;
	//m_misc = racers[playerRacerNum].distToStartCoord;
	//sprintf( buffer, "misc: %7.02f", m_misc );
	//OutputText( 0, m_vp.dwHeight - 35, buffer );

	//sprintf( buffer, "max verts: %d", maxVertices );
	//OutputText( 0, m_vp.dwHeight - 80, buffer );

/*
	sprintf( buffer, " Shadow quality: %s", ( bRenderShadowTexture ? "high" : "low" ) );
	OutputText( 0, m_vp.dwHeight - 50, buffer );

	sprintf( buffer, " AutoPitch: %s", ( bPlayerPitchAssist ? "on" : "off" ) );
	OutputText( 0, m_vp.dwHeight - 65, buffer );

	sprintf( buffer, " V-Sync: %s", ( m_bVSync ? "on" : "off" ) );
	OutputText( 0, m_vp.dwHeight - 80, buffer );

	sprintf( buffer, "lift: %7.02f", racers[playerRacerNum].liftIntensity );
	OutputText( 0, m_vp.dwHeight - 95, buffer );
*/
}




//-----------------------------------------------------------------------------
// Name: OutputText()
// Desc: Draws text on the window.
//-----------------------------------------------------------------------------
VOID CMyApplication::OutputText( DWORD x, DWORD y, TCHAR* str )
{
    HDC hDC;

    // Get a DC for the surface. Then, write out the buffer
    if( m_pddsRenderTarget )
    {
        if( SUCCEEDED( m_pddsRenderTarget->GetDC(&hDC) ) )
        {
            SetTextColor( hDC, RGB(255,255,255) );
            SetBkMode( hDC, TRANSPARENT );
            ExtTextOut( hDC, x, y, 0, NULL, str, lstrlen(str), NULL );
            m_pddsRenderTarget->ReleaseDC(hDC);
        }
    }

    // Do the same for the left surface (in case of stereoscopic viewing).
    if( m_pddsRenderTargetLeft )
    {
        if( SUCCEEDED( m_pddsRenderTargetLeft->GetDC( &hDC ) ) )
        {
            // Use a different color to help distinguish left eye view
            SetTextColor( hDC, RGB(255,0,255) );
            SetBkMode( hDC, TRANSPARENT );
            ExtTextOut( hDC, x, y, 0, NULL, str, lstrlen(str), NULL );
            m_pddsRenderTargetLeft->ReleaseDC(hDC);
        }
    }
}
 


//-----------------------------------------------------------------------------
// Name: Change3DEnvironment()
// Desc: Handles driver, device, and/or mode changes for the app.
//-----------------------------------------------------------------------------
HRESULT CMyApplication::Change3DEnvironment()
{
    HRESULT hr;
    static BOOL  bOldWindowedState = TRUE;
    static DWORD dwSavedStyle;
    static RECT  rcSaved;

    // Release all scene objects that will be re-created for the new device
    DeleteDeviceObjects();

    // Release framework objects, so a new device can be created
    if( FAILED( hr = m_pFramework->DestroyObjects() ) )
    {
//        DisplayFrameworkError( hr, MSGERR_APPMUSTEXIT );
//       SendMessage( m_hWnd, WM_CLOSE, 0, 0 );
        return hr;
    }

    // Check if going from fullscreen to windowed mode, or vice versa.
    if( bOldWindowedState != m_pDeviceInfo->bWindowed )
    {
        if( m_pDeviceInfo->bWindowed )
        {
            // Coming from fullscreen mode, so restore window properties
            SetWindowLong( m_hWnd, GWL_STYLE, dwSavedStyle );
            SetWindowPos( m_hWnd, HWND_NOTOPMOST, rcSaved.left, rcSaved.top,
                          ( rcSaved.right - rcSaved.left ), 
                          ( rcSaved.bottom - rcSaved.top ), SWP_SHOWWINDOW );
        }
        else
        {
            // Going to fullscreen mode, save/set window properties as needed
            dwSavedStyle = GetWindowLong( m_hWnd, GWL_STYLE );
            GetWindowRect( m_hWnd, &rcSaved );
            SetWindowLong( m_hWnd, GWL_STYLE, WS_POPUP|WS_SYSMENU|WS_VISIBLE );
        }

        bOldWindowedState = m_pDeviceInfo->bWindowed;
    }

    // Inform the framework class of the driver change. It will internally
    // re-create valid surfaces, a d3ddevice, etc.
    if( FAILED( hr = Initialize3DEnvironment() ) )
    {
//        DisplayFrameworkError( hr, MSGERR_APPMUSTEXIT );
//        SendMessage( m_hWnd, WM_CLOSE, 0, 0 );

			// if failed b/c of shadow test, retry and skip test
			if( bTestShadow ){
				bTestShadow = FALSE;
				bRenderShadowTexture = FALSE;
				return Change3DEnvironment();
			}
			else{
				return hr;
			}
    }

    // If the app is paused, trigger the rendering of the current frame
    if( FALSE == m_bFrameMoving )
    {
        m_bSingleStep = TRUE;
        startTime = timeGetTime() * 0.001f - elapsedTime;
    }
    
    m_pd3dDevice->SetRenderState( D3DRENDERSTATE_FOGENABLE, 	 TRUE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTFG_LINEAR ); 
	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTFG_LINEAR ); 

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: OnQuerySuspend()
// Desc: Called when the app receives a PBT_APMQUERYSUSPEND message, meaning
//       the computer is about to be suspended. At this point, the app should
//       save any data for open network connections, files, etc.., and prepare
//       to go into a suspended mode.
//-----------------------------------------------------------------------------
LRESULT CMyApplication::OnQuerySuspend( DWORD dwFlags )
{
    Pause(TRUE);

    return TRUE;
}




//-----------------------------------------------------------------------------
// Name: OnResumeSuspend()
// Desc: Called when the app receives a PBT_APMRESUMESUSPEND message, meaning
//       the computer has just resumed from a suspended state. At this point, 
//       the app should recover any data, network connections, files, etc..,
//       and resume running from when the app was suspended.
//-----------------------------------------------------------------------------
LRESULT CMyApplication::OnResumeSuspend( DWORD dwData )
{
    Pause(FALSE);

    return TRUE;
}
 



HRESULT CMyApplication::InitDeviceObjects(){

	HRESULT hr;

	// Set default render states
//	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_DITHERENABLE,	 TRUE );
	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_ZENABLE, 			 TRUE );
	m_pd3dDevice->SetRenderState(	D3DRENDERSTATE_CULLMODE,       D3DCULL_CW );
	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_ALPHABLENDENABLE, FALSE );
	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA );
	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA );
	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD );
	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_AMBIENT, D3DRGBA( 0.3, 0.3, 0.3, 0.0 ) );
	//m_pd3dDevice->SetRenderState( D3DRENDERSTATE_COLORVERTEX, TRUE ); 
	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL );
	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL );
	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL );
	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL );

	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESS, D3DTADDRESS_WRAP );
	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );

	//m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTFG_ANISOTROPIC ); 
	//m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTFG_ANISOTROPIC ); 

	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTFG_LINEAR ); 
	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTFG_LINEAR ); 

	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_BORDERCOLOR, 0x00000000 );


  // create viewport for primary rendering
	m_vp.dwX = 0;
	m_vp.dwY = 0;
	m_vp.dwWidth = m_pFramework->m_dwRenderWidth;
	m_vp.dwHeight = m_pFramework->m_dwRenderHeight;
	m_vp.dvMinZ = 0.0f;
	m_vp.dvMaxZ = 1.0f;
		
	// create projection matrix
	FLOAT fAspect = 0.75; // (FLOAT)m_vp.dwHeight / m_vp.dwWidth;
	D3DUtil_SetProjectionMatrix( m_matProj, VP_FOV, fAspect, VP_MINZ, VP_MAXZ );

	if( FAILED( hr = m_pd3dDevice->SetViewport( &m_vp ) ) ){
		MyAlert(&hr, "Unable to set viewport" );
		return E_FAIL;
	}

	m_pd3dDevice->SetTransform( D3DTRANSFORMSTATE_PROJECTION, &m_matProj );

/*
	DWORD letterboxHeight = m_pFramework->m_dwRenderHeight * 0.7;
	m_letterboxVp.dwX = 0;
	m_letterboxVp.dwY = 0;
	m_letterboxVp.dwWidth = m_pFramework->m_dwRenderWidth;
	m_letterboxVp.dwHeight = m_pFramework->m_dwRenderHeight;
	m_letterboxVp.dvMinZ = 0.0f;
	m_letterboxVp.dvMaxZ = 1.0f;
*/

//	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
//	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
//	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
//	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTFN_LINEAR );
//	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTFG_LINEAR );

	
		// Turn on fog

		FLOAT fFogStart = VP_MAXZ * 0.75;
		FLOAT fFogEnd 	= VP_MAXZ;
		m_pd3dDevice->SetRenderState( D3DRENDERSTATE_FOGENABLE, 	 TRUE );
		m_pd3dDevice->SetRenderState( D3DRENDERSTATE_FOGCOLOR,		 track.bgColor );
		m_pd3dDevice->SetRenderState( D3DRENDERSTATE_FOGSTART, *((DWORD *)(&fFogStart)) );
		m_pd3dDevice->SetRenderState( D3DRENDERSTATE_FOGEND,	 *((DWORD *)(&fFogEnd)) );
		//m_pd3dDevice->SetRenderState( D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_LINEAR );
		m_pd3dDevice->SetRenderState( D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_NONE );
		m_pd3dDevice->SetRenderState( D3DRENDERSTATE_FOGVERTEXMODE, D3DFOG_LINEAR );


	/*
	if( FAILED( hr = m_pd3dDevice->SetViewport( &m_vp ) ) ){
		MyAlert(&hr, "RenderToShadowTexture() > restoring primary rendering > SetViewport() failed" );
		return E_FAIL;
	}

	m_pd3dDevice->SetTransform( D3DTRANSFORMSTATE_PROJECTION, &m_matProj );
*/

	// OutputToLog( "Creating vertex buffers..." );

	// init vertex buffers
	D3DVERTEXBUFFERDESC vbDesc;
	ZeroMemory( &vbDesc, sizeof(D3DVERTEXBUFFERDESC) );
	vbDesc.dwSize = sizeof(D3DVERTEXBUFFERDESC);
	vbDesc.dwCaps = 0;
	vbDesc.dwFVF = TERRLVERTEXDESC;
	vbDesc.dwNumVertices = VERTEXBUFFERSIZE;

	// source VB for ProcessVertices should always be in system memory
	vbDesc.dwCaps |= D3DVBCAPS_SYSTEMMEMORY;

	if( FAILED( hr = m_pD3D->CreateVertexBuffer( &vbDesc, &track.terrVB, 0 ) ) ){
		MyAlert(&hr, "Unable to create terrain vertex buffer" );
		return E_FAIL;
	}

	ZeroMemory( &vbDesc, sizeof(D3DVERTEXBUFFERDESC) );
	vbDesc.dwSize = sizeof(D3DVERTEXBUFFERDESC);
	vbDesc.dwCaps = D3DVBCAPS_WRITEONLY;
	vbDesc.dwFVF = TERRTLVERTEXDESC;
	vbDesc.dwNumVertices = VERTEXBUFFERSIZE;

	// in system memory because writes (for type blends) are probably "erratic"
	vbDesc.dwCaps |= D3DVBCAPS_SYSTEMMEMORY;

	if( FAILED( hr = m_pD3D->CreateVertexBuffer( &vbDesc, &track.terrTransformedVB, 0 ) ) ){
		MyAlert(&hr, "Unable to create terrain transformed-vertex buffer" );
		return E_FAIL;
	}

	// OutputToLog( "Done\n" );

	// init sunlight
	D3DLIGHT7 d3dLight;
	ZeroMemory(&d3dLight, sizeof(D3DLIGHT7));

	d3dLight.dltType = D3DLIGHT_DIRECTIONAL;
	D3DCOLORVALUE white = { 1.0f, 1.0f, 1.0f, 1.0f };
	D3DCOLORVALUE black = { 0.1f, 0.1f, 0.1f, 1.0f };
	d3dLight.dcvDiffuse = white;
	d3dLight.dcvSpecular = white;
	d3dLight.dcvAmbient = black;

	d3dLight.dvDirection = D3DVECTOR( cosf( track.sunlightPitch ) * cosf( track.sunlightDirection ),
		-sinf( track.sunlightPitch ),
		cosf( track.sunlightPitch ) * sinf( track.sunlightDirection ) );

	if( FAILED( hr = m_pd3dDevice->SetLight(0, &d3dLight) ) ){
		MyAlert(&hr, "Unable to set Direct3D lighting" );
		return E_FAIL;
	}

	if( FAILED( hr = m_pd3dDevice->LightEnable(0, TRUE) ) ){
		MyAlert(&hr, "Could not enable Direct3D lighting" );
		return E_FAIL;
	}

	ZeroMemory( &m_defaultMtrl, sizeof(D3DMATERIAL7) );
	m_defaultMtrl.diffuse.r = 1.0f;
	m_defaultMtrl.diffuse.g = 1.0f;
	m_defaultMtrl.diffuse.b = 1.0f;
	m_defaultMtrl.diffuse.a = 1.0f;
	m_defaultMtrl.ambient = m_defaultMtrl.diffuse;
	
/*  
	= {
	1.0f, 1.0f, 1.0f, 1.0f,
	0.0f, 0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 0.0f, 1.0f,
	0.0f };
*/
	if( FAILED( InitStats() ) ){
		return E_FAIL;
	}

	// OutputToLog( "Initializing shadow texture..." );

	if( FAILED( InitShadowTexture() ) ){
		return E_FAIL;
	}

	CHAR msg[256];
	sprintf( msg, "Done (Shadow quality: %s)\n", ( bRenderShadowTexture ? "high" : "low" ) );
	// OutputToLog( msg );

	// OutputToLog( "Restoring textures..." );

	if( FAILED( RestoreTextures() ) ){
		return E_FAIL;
	}

	// OutputToLog( "Done\n" );

	if( !bRenderShadowTexture ){
		shadowTexture.pddsSurface = GetTexture( "staticshadow.bmp" )->m_pddsSurface;
	}

	return S_OK;
}




//-----------------------------------------------------------------------------
// Name: DeleteDeviceObjects()
// Desc: Called when the app is exitting, or the device is being changed,
//			 this function deletes any device dependant objects.
//-----------------------------------------------------------------------------
VOID CMyApplication::DeleteDeviceObjects(){

	SAFE_RELEASE( track.terrVB );
	SAFE_RELEASE( track.terrTransformedVB );

	ReleaseTextures();

	ReleaseShadowTexture();
}





HRESULT CMyApplication::UpdateGameState(){

	// Get the relative time, in seconds
	FLOAT oldElapsedTime = elapsedTime;
	elapsedTime = timeGetTime() * 0.001f - startTime;
	deltaTime = elapsedTime - oldElapsedTime;

	if( deltaTime <= 0.0f ){
/*
		CHAR msg[80];
		sprintf(msg, "deltaTime = %7.02f", deltaTime);
		MyAlert( NULL, msg );
*/
		deltaTime = 0.001f;
	}

	if( transitionState == TRANSITIONSTATE_FADEIN ){
		transitionTime -= deltaTime;

		if( transitionTime <= 0.0f ){
			transitionState = TRANSITIONSTATE_NORMAL;
		}
	}

	else if( transitionState == TRANSITIONSTATE_FADEOUT ){
		transitionTime -= deltaTime;

		if( transitionTime <= 0.0f ){
			gameState = newGameState;
			transitionState = TRANSITIONSTATE_FADEIN;
			transitionTime = FADEIN_DURATION;
			if( gameState == GAMESTATE_DEMORACE ||
				gameState == GAMESTATE_PLAYERRACE ){
				if( FAILED( RestartRace() ) ){
					return E_FAIL;
				}
			}
			else if( gameState == GAMESTATE_POSTRACE ){}
		}
	}

	else if( transitionState == TRANSITIONSTATE_NORMAL ){
		if( gameState == GAMESTATE_DEMORACE ){
			if( racers[cam.racerNum].currLap >= numLaps ){ // immediately restart demo race
				newGameState = GAMESTATE_DEMORACE;
				transitionState = TRANSITIONSTATE_FADEOUT;
				transitionTime = FADEOUT_DURATION;
			}
		}
		else if( gameState == GAMESTATE_PLAYERRACE ){
			if( racers[playerRacerNum].currLap >= numLaps ){ // go to post-race
				finishTime = elapsedTime;
				finishRacePosition = racers[playerRacerNum].racePosition;
				playerRacerNum = -1;
				gameState = GAMESTATE_POSTRACE;
				//transitionState = TRANSITIONSTATE_FADEOUT;
				//transitionTime = FADEOUT_DURATION;
			}
		}
		else if( gameState == GAMESTATE_POSTRACE ){
			if( racers[cam.racerNum].currLap >= numLaps + 1 ){ // do a victory lap
				newGameState = GAMESTATE_DEMORACE;
				transitionState = TRANSITIONSTATE_FADEOUT;
				transitionTime = FADEOUT_DURATION;
			}
		}
	}

	return S_OK;
}




HRESULT CMyApplication::RestartRace(){

	startTime = timeGetTime() * 0.001f + RACE_INTRO_DURATION[ gameState ] + 1.0f;
	elapsedTime = -RACE_INTRO_DURATION[ gameState ];
	deltaTime = 0.001f;

	if( FAILED( InitRacers() ) ){ // currently, this always succeeds
		return E_FAIL;
	}

	if( gameState == GAMESTATE_DEMORACE ){

		playerRacerNum = -1;

		cam.racerNum = 1;

		// bypassing normal cam switch handling within Set-Camera()
		cam.mode = CAMERAMODE_FIXEDPOSITION;
		cam.bReadyToSwitch = FALSE;
		// cam.switchBaseTime = elapsedTime - CAMERA_SWITCH_DELAY;
		cam.fixedPositionNum = 0;
		cam.pos = fixedCameraPositions[cam.fixedPositionNum] + D3DVECTOR( -200, 30, 60 );
		cam.nearestDist = 100000.0f;

		if( !m_bAttemptingInitialization ){
			PlayBuffer( FALSE, 425000 * 4 ); // offset to near end of track (4 bytes/sample)
		}
	}

	else if( gameState == GAMESTATE_PLAYERRACE ){

		playerRacerNum = MAX_RACERS - 1;

		cam.racerNum = playerRacerNum;

		// bypassing normal cam switch handling within Set-Camera()
		cam.mode = CAMERAMODE_NORMAL;
		cam.bReadyToSwitch = FALSE;

		cam.fixedPositionNum = 1; // anything but zero, so that zero gets picked in post-race
		cam.offset = D3DVECTOR(
			-50 * cosf( racers[cam.racerNum].yaw ),
			10,
			-50 * sinf( racers[cam.racerNum].yaw ) );

		StopBuffer();
	}

	return S_OK;
}



VOID CMyApplication::UpdateStreamingSoundBuffer(){

	FLOAT volume;
	static FLOAT oldVolume;

	if( gameState == GAMESTATE_DEMORACE ){
		if( elapsedTime < 0.0f ){
			volume = pow( 1.0f - ( -elapsedTime / RACE_INTRO_DURATION[GAMESTATE_DEMORACE] ), 0.15f );
		}
		else{
			volume = 1.0f;
		}
	}
	else if( gameState == GAMESTATE_PLAYERRACE ){
		CRacer* pRacer = &racers[playerRacerNum];
		if( pRacer->currLap == numLaps - 1 ){

			volume = 0.9f; // pow( 1.0f - min( 1.0f, (FLOAT)pRacer->distToStartCoord / track.trackLength ), 0.2f ) * 0.93f;

            #ifdef APP_GAME
			    if( !IsStreamingBufferPlaying() ){
			    	PlayBuffer( FALSE, 0 );
			    }
            #endif
		}
		else{
			volume = 0.0f;
		}
	}
	else if( gameState == GAMESTATE_POSTRACE ){
		volume = 1.0f;
	}

	if( transitionState == TRANSITIONSTATE_FADEOUT ){
		// fade out music volume from where-ever its at
		volume *= pow( transitionTime / FADEOUT_DURATION, 0.3f );
	}

	if( volume != oldVolume ){
		AdjustStreamingSoundBuffer( volume );
		oldVolume = volume;
	}
}



// make all moving shots begin with cam focusing exactly on craft

const D3DVECTOR baseFixedCameraPositions[ NUM_FIXED_CAMERA_POSITIONS ] = {
	D3DVECTOR( 340, 18, 630 ),
	D3DVECTOR( 444, 7, 616 ), // 733 ),
	D3DVECTOR( 627, 21, 423 ), // fix this shot

	D3DVECTOR( 356, 4, 106 ),
//	D3DVECTOR( 463, 40, 101 ),
	D3DVECTOR( 644, 30, 247 ), // little jump

	D3DVECTOR( 319, 88, 180 ), // big jump
	D3DVECTOR( 79, 9, 165 ),

	D3DVECTOR( 85, 39, 435 ),
	D3DVECTOR( 88, 10, 693 )
};

const FLOAT fixedCameraStartDists[ NUM_FIXED_CAMERA_POSITIONS ] = {
	85,
	59,
	70,

	60,
//	70,
	77,

	70,
	80,

	84,
	75
};

const FLOAT fixedCameraEndDists[ NUM_FIXED_CAMERA_POSITIONS ] = {
	100,
	101,
	41,

	44,
//	60,
	75,

	58,
	58,

	48,
	60
};


const DWORD switchToMovingCamera[ NUM_FIXED_CAMERA_POSITIONS ] = {
	CAMERAMODE_NORMAL,
	CAMERAMODE_NORMAL,
	CAMERAMODE_REARVIEW,

	CAMERAMODE_NORMAL,
//	CAMERAMODE_ORBITRACER,
	CAMERAMODE_REARVIEW,

	CAMERAMODE_NORMAL,
	CAMERAMODE_REARVIEW,

	CAMERAMODE_NORMAL,
	CAMERAMODE_REARVIEW,
};


// normal camera offsets (user selects)
const FLOAT normalCameraHorzOffsets[ NUM_NORMAL_CAMERA_OFFSETS ] = {
	10, // 20, // 20
	20, // 20
	20,
	20
};

const FLOAT normalCameraVertOffsets[ NUM_NORMAL_CAMERA_OFFSETS ] = {
	4, //18,
	24,
	30,
	36
};


VOID CMyApplication::InitFixedCameraPositions(){

	for(DWORD pos=0; pos < NUM_FIXED_CAMERA_POSITIONS; pos++){
		fixedCameraPositions[pos] = baseFixedCameraPositions[pos];
		fixedCameraPositions[pos].y += 
			track.terrCoords[(DWORD)fixedCameraPositions[pos].z * track.width + (DWORD)fixedCameraPositions[pos].x].height * TERR_HEIGHT_SCALE;
	}
}




VOID CMyApplication::SetCamera( FLOAT deltaTime ){
	
	HRESULT hr;
	
	D3DVECTOR newOffset, vLookatPt, sep, normSep;
	FLOAT sepDist, scaledMaxVel, correctionDist, dist;
	static D3DVECTOR oldCamPos = cam.pos;
	
	FLOAT racerVelMag = Magnitude( racers[cam.racerNum].vel ) / RACER_REAL_MAXVEL;

	D3DVECTOR vUpVec		= D3DVECTOR( 0.0f, 1.0f, 0.0f );

	FLOAT fixedPositionDist = 100000.0f;


	if( cam.bReadyToSwitch && ( elapsedTime - cam.switchBaseTime >= CAMERA_SWITCH_DELAY ) ){

		cam.mode = cam.newMode;
		cam.bReadyToSwitch = FALSE;

		if( cam.mode == CAMERAMODE_FIXEDPOSITION ){
			cam.fixedPositionNum = cam.newFixedPositionNum;
			cam.pos = fixedCameraPositions[cam.fixedPositionNum];
			vLookatPt = racers[cam.racerNum].pos;
			cam.nearestDist = 100000;
			oldCamPos = cam.pos;
		}
		if( cam.mode == CAMERAMODE_NORMAL ){
			cam.offset = D3DVECTOR( 
				-( 15 ) * cosf( racers[cam.racerNum].yaw ),
				32,
				-( 15 ) * sinf( racers[cam.racerNum].yaw ) );
			oldCamPos = cam.pos + cam.offset;
		}
		else{
			cam.offset = D3DVECTOR( 
				( 90 ) * cosf( racers[cam.racerNum].yaw ),
				50,
				( 90 ) * sinf( racers[cam.racerNum].yaw ) );
			oldCamPos = cam.pos + cam.offset;
		}
	}


	if( cam.mode == CAMERAMODE_NORMAL ){
		cam.targetOffset = D3DVECTOR( 

			-normalCameraHorzOffsets[ cam.offsetNum ] * cosf( racers[cam.racerNum].yaw ),
			normalCameraVertOffsets[ cam.offsetNum ],
			-normalCameraHorzOffsets[ cam.offsetNum ] * sinf( racers[cam.racerNum].yaw ) );

/*
			-7 * cosf( -racers[cam.racerNum].yaw + g_PI/4 ),
			10,
			-7 * sinf( -racers[cam.racerNum].yaw + g_PI/4 ) );
*/		
		static FLOAT height = normalCameraVertOffsets[ cam.offsetNum ] *
			CAMERA_LEADDIST / ( normalCameraHorzOffsets[ cam.offsetNum ] + CAMERA_LEADDIST );

		FLOAT weight = min( 1.0f, 10.0f * deltaTime );
		height = height * ( 1.0f - weight ) + 
			( normalCameraVertOffsets[ cam.offsetNum ] * CAMERA_LEADDIST / 
			( normalCameraHorzOffsets[ cam.offsetNum ] + CAMERA_LEADDIST ) ) * weight;

		//if( height > normalCameraVertOffsets[ cam.offsetNum ] ){
		//	height = normalCameraVertOffsets[ cam.offsetNum ];
		//}


		vLookatPt = D3DVECTOR( 
			racers[cam.racerNum].pos.x,
			racers[cam.racerNum].pos.y + height,
			racers[cam.racerNum].pos.z );
	}

	else if( cam.mode == CAMERAMODE_ORBITRACER ){

		cam.targetOffset = D3DVECTOR( 
			( 15 ) * cosf( -racers[cam.racerNum].yaw + g_PI ),
			14,
			( 15 ) * sinf( -racers[cam.racerNum].yaw + g_PI ) );
		
		vLookatPt = D3DVECTOR( 
			racers[cam.racerNum].pos.x,
			racers[cam.racerNum].pos.y + 0.0f,
			racers[cam.racerNum].pos.z );
	}

	else if( cam.mode == CAMERAMODE_REARVIEW ){

		cam.targetOffset = D3DVECTOR( 
			( 15 ) * cosf( racers[cam.racerNum].yaw ),
			7,
			( 15 ) * sinf( racers[cam.racerNum].yaw ) );
		
		vLookatPt = D3DVECTOR( 
			racers[cam.racerNum].pos.x,
			racers[cam.racerNum].pos.y + 3.0f,
			racers[cam.racerNum].pos.z );
	}


	if( cam.mode == CAMERAMODE_NORMAL || cam.mode == CAMERAMODE_ORBITRACER ||
		cam.mode == CAMERAMODE_REARVIEW ){


		newOffset = cam.offset;
		
		sep = cam.targetOffset - cam.offset;
		
		sepDist = Magnitude( sep );
		
		scaledMaxVel = CAMERA_MAXVEL * pow( deltaTime, 0.5 ) * 0.12;
		
		if( sepDist < scaledMaxVel ){
			newOffset = cam.targetOffset;
		}
		else{
			newOffset += ( sep / sepDist ) * scaledMaxVel;
		}

		//FLOAT weight = min( 1.0f, 20.0f * deltaTime );
		//newOffset = cam.offset * ( 1.0f - weight ) + cam.targetOffset * weight;

/*
		// camera-terrain collision
		D3DVECTOR tempPos = racers[cam.racerNum].pos + cam.offset;
		
		if( tempPos.x >= 0 && tempPos.z >= 0 && tempPos.x < track.width - 1 && tempPos.z < track.depth - 1 ){
			
			if( fabs( newOffset.z ) > g_EPSILON || fabs( newOffset.x ) > g_EPSILON ){
				cam.yaw = atan2( newOffset.z, newOffset.x );
			}

			D3DVECTOR vTempDir = Normalize( 
			//	D3DVECTOR( cosf( cam.yaw ), -0.1f,  sinf( cam.yaw ) ) );
				newOffset - cam.offset );

			FLOAT mag = Magnitude( newOffset - cam.offset );

			if( mag > 0.0f ){
				dist = DistToTerrain( tempPos, vTempDir, mag );
			//if( dist < mag ){

				DWORD index = floor( tempPos.z ) * track.width + floor( tempPos.x );
				FLOAT slope1 = (FLOAT)( track.terrCoords[index + track.width + 1].height - 
					track.terrCoords[index].height ) * TERR_HEIGHT_SCALE;
				FLOAT slope2 = (FLOAT)( track.terrCoords[index + 1].height -
					track.terrCoords[index + track.width].height ) * TERR_HEIGHT_SCALE;

				D3DVECTOR vDiag1 = D3DVECTOR( 1.0f, slope1, 1.0f );
				D3DVECTOR vDiag2 = D3DVECTOR( 1.0f, slope2, -1.0f );

				D3DVECTOR vGroundNormal = Normalize( CrossProduct( vDiag1, vDiag2 ) );

			//	newOffset -=  * dist; 
			//}

				if( dist < 0 || dist > mag ){
					DWORD blah=0;
				}

				cam.offset += vTempDir * dist;
			}
		}
*/
		D3DVECTOR tempPos = racers[cam.racerNum].pos + newOffset;
	
		if( tempPos.x >= 0 && tempPos.z >= 0 && tempPos.x < track.width - 1 && tempPos.z < track.depth - 1 ){

			const FLOAT correctionMag[4] = { 0.12f, 0.0f, 0.03f, 0.03f };
			static FLOAT distsToTerrain[5] = { 
				30.0f, 30.0f, 30.0f, 30.0f, 30.0f };

			FLOAT weight = min( 1.0f, 10.0f * deltaTime );

			if( fabs( newOffset.z ) > g_EPSILON || fabs( newOffset.x ) > g_EPSILON ){
				cam.yaw = atan2( newOffset.z, newOffset.x );
			}

			FLOAT angleOffset = -0.25 * g_PI;
			for(DWORD angleNum=0; angleNum < 5; angleNum++, angleOffset += 0.125 * g_PI ){
				
				D3DVECTOR vTempDir = Normalize( 
					D3DVECTOR( cosf( cam.yaw + angleOffset ), -0.2f,  sinf( cam.yaw + angleOffset ) ) );
				
				dist = distsToTerrain[angleNum] = distsToTerrain[angleNum] * ( 1.0f - weight ) + 
					DistToTerrain( tempPos, vTempDir, 30.0f ) * weight;

				if( dist < 30.0f ){
					
					//if( dist < 1.0f ){ dist = 1.0f; }
					
					correctionDist = ( 30.0f - dist ) * ( 30.0f - dist ) * correctionMag[cam.mode]; // - ( 5.0f * dist / 60.0f ); // ( dist * dist );
					
					if( correctionDist > CAMERA_MAXVEL * 0.5f ){
						correctionDist = CAMERA_MAXVEL * 0.5f;
					}
					
					newOffset -= vTempDir * correctionDist * 0.015f; //deltaTime;
				}

			}
		}

		const FLOAT modeWeight[4] = { 10.0f, 0.0f, 10.0f, 10.0f };

		// average offset over several frames
		FLOAT weight = min( 1.0f, modeWeight[cam.mode] * pow( deltaTime, 0.5 ) * 0.12 );
		cam.offset = cam.offset * ( 1.0f - weight ) + newOffset * weight;

		if( cam.mode == CAMERAMODE_NORMAL ){
			// keep camera behind craft;
			dist = 1.0f + cam.offset.x * cosf( racers[cam.racerNum].yaw ) + cam.offset.z * sinf( racers[cam.racerNum].yaw );
			
			if( dist > 0.0f ){
				cam.offset.x -= cosf( racers[cam.racerNum].yaw ) * dist;
				cam.offset.z -= sinf( racers[cam.racerNum].yaw ) * dist;
			}
		}

		cam.pos = racers[cam.racerNum].pos + cam.offset;
	}

	else if( cam.mode == CAMERAMODE_FIXEDPOSITION ){

		if( cam.pos.x != fixedCameraPositions[cam.fixedPositionNum].x ||
				cam.pos.y != fixedCameraPositions[cam.fixedPositionNum].y ||
				cam.pos.z != fixedCameraPositions[cam.fixedPositionNum].z ){

			// intro cam movement
			/* 
			sep = fixedCameraPositions[cam.fixedPositionNum] - cam.pos;

			FLOAT vel = 35.0f * deltaTime;
			if( Magnitude( sep ) > vel ){
				cam.pos += Normalize( sep ) * vel;
			}
			else{
				cam.pos = fixedCameraPositions[cam.fixedPositionNum];
			}
*/
			FLOAT weight = min( 1.0f, 0.4f * deltaTime );
			cam.pos = cam.pos * ( 1.0f - weight ) + fixedCameraPositions[cam.fixedPositionNum] * weight;
		}

		vLookatPt = racers[cam.racerNum].pos;

		fixedPositionDist = Magnitude( cam.pos - vLookatPt );

		if( fixedPositionDist < cam.nearestDist ){
			cam.nearestDist = fixedPositionDist;
		}
		else if( !cam.bReadyToSwitch && 
			( fixedPositionDist > cam.nearestDist + fixedCameraEndDists[cam.fixedPositionNum] ) ){

			cam.newMode = switchToMovingCamera[cam.fixedPositionNum];
			cam.switchBaseTime = elapsedTime;
			cam.bReadyToSwitch = TRUE;

            #ifdef APP_GAME
			    if( !IsStreamingBufferPlaying() ){
			    	PlayBuffer( FALSE, 0 );
			    }
            #endif
		}
	}


	// check to see if time for next fixed-pos shot (regardless of current cam mode)
	if( gameState == GAMESTATE_DEMORACE || gameState == GAMESTATE_POSTRACE ){
		
		if( !cam.bReadyToSwitch ){
			for(DWORD pos=0; pos < NUM_FIXED_CAMERA_POSITIONS; pos++){
				
				if( pos != cam.fixedPositionNum ){
					dist = Magnitude( fixedCameraPositions[pos] - racers[cam.racerNum].pos );
					
					if( dist < fixedCameraStartDists[pos] && dist < fixedPositionDist ){
						
						cam.newMode = CAMERAMODE_FIXEDPOSITION;
						cam.switchBaseTime = elapsedTime;
						cam.bReadyToSwitch = TRUE;
						
						cam.newFixedPositionNum = pos;
						
                        #ifdef APP_GAME
						    if( !IsStreamingBufferPlaying() ){
					    		PlayBuffer( FALSE, 0 );
					    	}
                        #endif
					}
				}
			}
		}
	}

/* screwy 1st-person cam
	D3DMath_VectorMatrixMultiply( vUpVec, D3DVECTOR( 0, 1, 0 ), racers[cam.racerNum].matRot );

	cam.pos = racers[cam.racerNum].pos;

	vLookatPt = racers[cam.racerNum].pos + 10.0f * D3DVECTOR( cosf( racers[cam.racerNum].yaw ) * cosf( racers[cam.racerNum].pitch ),
			sinf( racers[cam.racerNum].pitch ),
			sinf( racers[cam.racerNum].yaw ) * cosf( racers[cam.racerNum].pitch ) ); // +
			// 4.0f * vUpVec;;
*/

/* mouse-look cam
	if( cam.mode == CAMERAMODE_NORMAL ){
		
		D3DMATRIX matRot, matTemp;
		
		D3DUtil_SetIdentityMatrix(matRot);
		
		// Produce and combine the rotation matrices.
		//D3DUtil_SetRotateXMatrix(matTemp, racers[cam.racerNum].roll);     // roll
		//D3DMath_MatrixMultiply(matRot,matRot,matTemp);
		D3DUtil_SetRotateZMatrix(matTemp, racers[cam.racerNum].lookPitch);    // pitch
		D3DMath_MatrixMultiply(matRot,matRot,matTemp);
		D3DUtil_SetRotateYMatrix(matTemp, -racers[cam.racerNum].yaw);  // yaw -- negative because world-y's sign is screwy
		D3DMath_MatrixMultiply(matRot,matRot,matTemp);
		
		D3DVECTOR vTemp;
		D3DMath_VectorMatrixMultiply( vTemp, D3DVECTOR( -(10 + (FLOAT)cam.offsetNum * 2.5), 4, 0 ), matRot );
		
		cam.pos = racers[cam.racerNum].pos + vTemp;
		
		D3DMath_VectorMatrixMultiply( vTemp, D3DVECTOR( 0, 4, 0 ), matRot );
		vLookatPt = racers[cam.racerNum].pos + vTemp;
	}
*/
	SetViewMatrix(
		m_matView,
		cam.pos,
		vLookatPt,
		vUpVec );
		
	if( FAILED( hr = m_pd3dDevice->SetTransform( D3DTRANSFORMSTATE_VIEW, &m_matView ) ) ){
		MyAlert( &hr, "Unable to set view matrix" );
		//return E_FAIL;
	}
		
	cam.dir = Normalize( vLookatPt - cam.pos );

#ifdef APP_GAME
	// update sound
	UpdateListener( cam.dir, cam.pos, ( cam.pos - oldCamPos ) / deltaTime );
#endif
	oldCamPos = cam.pos;

}




FLOAT DistToCam( D3DVECTOR pos ){

	return Magnitude( pos - g_pMyApp->GetCam().pos );
}





VOID CMyApplication::SetVisibleTerrainCoords(){

	D3DMATRIX matSet, matInvSet;
	D3DVECTOR vScreen, vWorld, vInc, vPos;
	FLOAT sx, sy, sxInc, syInc, mag, zInc, zDist;
	DWORD numTestPoints;
	INT32 testX, testY, testZ, incMag;
	BOOL outOfBounds;

	D3DMath_MatrixMultiply( matSet, m_matView, m_matProj );

	FLOAT junk;
	D3DXMatrixInverse( (D3DXMATRIX*)&matInvSet, &junk, (D3DXMATRIX*)&matSet );

	// set an invalid RectRegion, guaranteed to be overridden by point tests below
	track.visibleTerrCoords.Set( track.width - 1, track.depth - 1, 0, 0 );

	vScreen.z = 0.0;

	incMag = 10.0f;
	numTestPoints = 24;
	sxInc = 2.0f / numTestPoints;
	syInc = 2.0f / ( numTestPoints / 3 ); /// sides are not as crucial (plus aspect ratio)


	for(sy=-1; sy <= 1; sy += syInc ){
		for(sx=-1; sx <= 1; sx += 2 ){ // test left and right edges

			vScreen.x = sx;
			vScreen.y = sy;
			
			D3DMath_VectorMatrixMultiply( vWorld, vScreen, matInvSet );
			
			vInc = Normalize( vWorld - cam.pos );
			zInc = DotProduct( vInc, cam.dir );

			vPos = cam.pos + vInc * ( 20.0f + sy * 20.0f ); // start further at top test points

			zDist = zInc * ( 20.0f + sy * 20.0f );

			vInc *= incMag;
			zInc *= incMag;

			vInc.y /= TERR_HEIGHT_SCALE;
			vPos.y /= TERR_HEIGHT_SCALE;

			outOfBounds = FALSE;			

			while( zDist < VP_MAXZ ){

				testX = vPos.x + 0.5;
				testY = vPos.y + 0.5;
				testZ = vPos.z + 0.5;

				if( testX < 0 ){
					if( vInc.x <= 0 ){
						break;
					}
					else{
						outOfBounds = TRUE;
					}
				}
				else if( testX >= track.width ){
					if( vInc.x >= 0 ){
						break;
					}
					else{
						outOfBounds = TRUE;
					}
				}

				if( testZ < 0 ){
					if( vInc.z <= 0 ){
						break;
					}
					else{
						outOfBounds = TRUE;
					}
				}
				else if( testZ >= track.width ){
					if( vInc.z >= 0 ){
						break;
					}
					else{
						outOfBounds = TRUE;
					}
				}

				if( !outOfBounds ){
					if( track.terrCoords[testZ * track.width + testX].height > testY ){
						break;
					}
				}
				else{
					if( testY < 0 ){
						break;
					}
					outOfBounds = FALSE; // reset for next test;
				}

				vPos += vInc;
				zDist += zInc;

			}
			
			if( vPos.x < track.visibleTerrCoords.startX ){ track.visibleTerrCoords.startX = vPos.x; }
			if( vPos.x > track.visibleTerrCoords.endX ){ track.visibleTerrCoords.endX = vPos.x; }
			if( vPos.z < track.visibleTerrCoords.startZ ){ track.visibleTerrCoords.startZ = vPos.z; }
			if( vPos.z > track.visibleTerrCoords.endZ ){ track.visibleTerrCoords.endZ = vPos.z; }
		}
	}

	for(sx=-1 + sxInc; sx < 1; sx += sxInc ){ // don't redundantly test corners
		for(sy=-1; sy <= 1; sy += 2 ){ // test top and bottom edges

			vScreen.x = sx;
			vScreen.y = sy;
			
			D3DMath_VectorMatrixMultiply( vWorld, vScreen, matInvSet );
			
			vInc = Normalize( vWorld - cam.pos );
			zInc = DotProduct( vInc, cam.dir );

			vPos = cam.pos + vInc * ( 20.0f + sy * 20.0f ); // start further at top test points

			zDist = zInc * ( 20.0f + sy * 20.0f );

			vInc *= incMag;
			zInc *= incMag;

			vInc.y /= TERR_HEIGHT_SCALE;
			vPos.y /= TERR_HEIGHT_SCALE;

			outOfBounds = FALSE;			

			while( zDist < VP_MAXZ ){

				testX = vPos.x + 0.5;
				testY = vPos.y + 0.5;
				testZ = vPos.z + 0.5;

				if( testX < 0 ){
					if( vInc.x <= 0 ){
						break;
					}
					else{
						outOfBounds = TRUE;
					}
				}
				else if( testX >= track.width ){
					if( vInc.x >= 0 ){
						break;
					}
					else{
						outOfBounds = TRUE;
					}
				}

				if( testZ < 0 ){
					if( vInc.z <= 0 ){
						break;
					}
					else{
						outOfBounds = TRUE;
					}
				}
				else if( testZ >= track.width ){
					if( vInc.z >= 0 ){
						break;
					}
					else{
						outOfBounds = TRUE;
					}
				}

				if( !outOfBounds ){
					if( track.terrCoords[testZ * track.width + testX].height > testY ){
						break;
					}
				}
				else{
					if( testY < 0 ){
						break;
					}
					outOfBounds = FALSE; // reset for next test;
				}

				vPos += vInc;
				zDist += zInc;

			}
			
			if( vPos.x < track.visibleTerrCoords.startX ){ track.visibleTerrCoords.startX = vPos.x; }
			if( vPos.x > track.visibleTerrCoords.endX ){ track.visibleTerrCoords.endX = vPos.x; }
			if( vPos.z < track.visibleTerrCoords.startZ ){ track.visibleTerrCoords.startZ = vPos.z; }
			if( vPos.z > track.visibleTerrCoords.endZ ){ track.visibleTerrCoords.endZ = vPos.z; }
		}
	}
	
	DWORD padding = 6;
	track.visibleTerrCoords.startX -= padding;
	track.visibleTerrCoords.startZ -= padding;
	track.visibleTerrCoords.endX += padding;
	track.visibleTerrCoords.endZ += padding;

	track.visibleTerrCoords.Clip( 0, 0, track.width - 1, track.depth - 1 );

	track.renderedTriGroups.Set( track.visibleTerrCoords.startX >> TRIGROUP_WIDTHSHIFT,
		track.visibleTerrCoords.startZ >> TRIGROUP_WIDTHSHIFT,
		( track.visibleTerrCoords.endX - 1 ) >> TRIGROUP_WIDTHSHIFT,
		( track.visibleTerrCoords.endZ - 1 ) >> TRIGROUP_WIDTHSHIFT );

	track.renderedTerrCoords.Set( track.renderedTriGroups.startX << TRIGROUP_WIDTHSHIFT,
		track.renderedTriGroups.startZ << TRIGROUP_WIDTHSHIFT,
		( ( track.renderedTriGroups.endX + 1 ) << TRIGROUP_WIDTHSHIFT ),
		( ( track.renderedTriGroups.endZ + 1 ) << TRIGROUP_WIDTHSHIFT ));

	while( ( track.renderedTriGroups.endX - track.renderedTriGroups.startX + 1 ) *
		( track.renderedTriGroups.endZ - track.renderedTriGroups.startZ + 1 ) > MAX_RENDERED_GROUPS ){

		if( track.visibleTerrCoords.endX - track.visibleTerrCoords.startX > 20 ){
			track.visibleTerrCoords.startX++;
			track.visibleTerrCoords.endX--;
		}
		if( track.visibleTerrCoords.endZ - track.visibleTerrCoords.startZ > 20 ){
			track.visibleTerrCoords.startZ++;
			track.visibleTerrCoords.endZ--;
		}

		track.renderedTriGroups.Set( track.visibleTerrCoords.startX >> TRIGROUP_WIDTHSHIFT,
			track.visibleTerrCoords.startZ >> TRIGROUP_WIDTHSHIFT,
			( track.visibleTerrCoords.endX - 1 ) >> TRIGROUP_WIDTHSHIFT,
			( track.visibleTerrCoords.endZ - 1 ) >> TRIGROUP_WIDTHSHIFT );

		track.renderedTerrCoords.Set( track.renderedTriGroups.startX << TRIGROUP_WIDTHSHIFT,
			track.renderedTriGroups.startZ << TRIGROUP_WIDTHSHIFT,
			( ( track.renderedTriGroups.endX + 1 ) << TRIGROUP_WIDTHSHIFT ),
			( ( track.renderedTriGroups.endZ + 1 ) << TRIGROUP_WIDTHSHIFT ));
	}

}






VOID CMyApplication::DisplayAlert( HRESULT* phr, CHAR* desc ){
	CHAR msg[400];

/*
  if( phr != NULL ){

		CHAR d3dError[200];
		D3DXGetErrorString( *phr, 200, d3dError );

		sprintf( msg, "Error: %s\n\nRelated error code: %s\n\nPress OK to exit Hover.", desc, d3dError );
	}
	else{
		sprintf( msg, "Error: %s\n\nPress OK to exit Hover.", desc );
	}

	if( g_pMyApp->GetActiveAndReady() ){
		g_pMyApp->Pause(TRUE);

		g_pMyApp->GetDeviceInfo()->bWindowed = TRUE;
							
		if( FAILED( g_pMyApp->Change3DEnvironment() ) ){
			return;
		}
	}

//	DestroyWindow( g_pMyApp->GetWindowHandle() );

	//SendMessage( g_pMyApp->GetWindowHandle(), WM_SIZE, SIZE_MAXHIDE, 0 );

	MessageBox( NULL, msg, "Hover Error", MB_OK | MB_ICONWARNING );

	if( g_pMyApp->GetActiveAndReady() ){
		g_pMyApp->Pause(FALSE);
	}
*/
/*
 if( phr != NULL ){

		CHAR d3dError[200];
		D3DXGetErrorString( *phr, 200, d3dError );

		sprintf( msg, " [Error: %s\nRelated error code: %s] ", desc, d3dError );
	}
	else{
		sprintf( msg, " [Error: %s] ", desc );
	}

	// OutputToLog( msg );
*/
	HDC hDC;

	BOOL temp = GetUpdateRect(
		m_hWnd,       // handle to window
		NULL,   // update rectangle coordinates
		FALSE      // erase state
	);

	if( m_hWnd && ( hDC = GetDC( m_hWnd ) ) && GetUpdateRect( m_hWnd, NULL, FALSE	) ){

		SetTextColor( hDC, RGB(255,50,0) );
		SetBkColor( hDC, RGB(255,255,255) ); 
		SetBkMode( hDC, OPAQUE );

		sprintf( msg, " Error: %s", desc );
		ExtTextOut( hDC, 0, 0, 0, NULL, msg, lstrlen(msg), NULL );

		if( phr != NULL ){
			CHAR d3dError[200];
			D3DXGetErrorString( *phr, 200, d3dError );
			sprintf( msg, " Related error code: %s", d3dError );
			ExtTextOut( hDC, 0, 15, 0, NULL, msg, lstrlen(msg), NULL );
		}

		if( !m_bAttemptingInitialization ){
			strcpy( msg, " Press space bar to exit Hover." );
			ExtTextOut( hDC, 0, 30, 0, NULL, msg, lstrlen(msg), NULL );
		}
		else{
			strcpy( msg, " Press space bar to continue." );
			ExtTextOut( hDC, 0, 30, 0, NULL, msg, lstrlen(msg), NULL );
		}

		while( !( GetAsyncKeyState( VK_SPACE ) & 0x8000 ) ){}

		ReleaseDC( m_hWnd, hDC );
	}

	else{
		if( phr != NULL ){
			
			CHAR d3dError[200];
			D3DXGetErrorString( *phr, 200, d3dError );
			
			sprintf( msg, "Error: %s\nRelated error code: %s\n\n", desc, d3dError );
		}
		else{
			sprintf( msg, "Error: %s\n\n", desc );
		}

		if( m_bAttemptingInitialization ){
			strcat( msg, "Press OK to continue." );
		}
		else{
			strcat( msg, "Press OK to exit Hover." );
		}

		MessageBox( NULL, msg, "Hover Error", MB_OK | MB_ICONWARNING );
	}

	if( phr != NULL ){
		CHAR d3dError[200];
		D3DXGetErrorString( *phr, 200, d3dError );
		sprintf( msg, "\nError: %s\nRelated error code: %s\n\n", desc, d3dError );
	}
	else{
		sprintf( msg, "\nError: %s\n\n", desc );
	}

	// OutputToLog( msg );
}
	



VOID MyAlert( HRESULT* phr, CHAR* desc ){

	g_pMyApp->DisplayAlert( phr, desc );
}



/*
VOID OutputToLog( CHAR* msg ){

	HANDLE logFile;

	if( ( logFile = CreateFile( "hoverlog.txt", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL, NULL ) ) == INVALID_HANDLE_VALUE ){
		MyAlert( NULL, msg );
		return;
	}

	SetFilePointer(logFile, 0, NULL, FILE_END );

	DWORD numBytes;
	if( !WriteFile( logFile, msg, strlen( msg ), &numBytes, NULL ) ){
		MyAlert( NULL, msg );
		return;
	}

	CloseHandle( logFile );
}

*/


