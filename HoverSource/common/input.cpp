

#include "common.h"

//#define INITGUID

LPDIRECTINPUT       g_pKeyboardDI       = NULL;         
LPDIRECTINPUTDEVICE g_pKeyboard = NULL;     
BOOL                g_bActive   = TRUE;     

IDirectInput*           g_pMouseDI       = NULL;
IDirectInputDevice*     g_pMouse    = NULL;
HINSTANCE               g_hInst     = NULL;


//-----------------------------------------------------------------------------
// Name: InitDirectInput()
// Desc: Initialize the DirectInput variables.
//-----------------------------------------------------------------------------
HRESULT InitDirectInput( HWND hWnd )
{
    HRESULT hr;

    // Register with the DirectInput subsystem and get a pointer
    // to a IDirectInput interface we can use.
    hr = DirectInputCreate( (HINSTANCE)GetWindowLong( hWnd, GWL_HINSTANCE ),
		                    DIRECTINPUT_VERSION, &g_pKeyboardDI, NULL );
    if( FAILED(hr) ) 
        return hr;

    // Obtain an interface to the system keyboard device.
    hr = g_pKeyboardDI->CreateDevice( GUID_SysKeyboard, &g_pKeyboard, NULL );
    if( FAILED(hr) ) 
        return hr;

    // Set the data format to "keyboard format" - a predefined data format 
    //
    // A data format specifies which controls on a device we
    // are interested in, and how they should be reported.
    //
    // This tells DirectInput that we will be passing an array
    // of 256 bytes to IDirectInputDevice::GetDeviceState.
    hr = g_pKeyboard->SetDataFormat( &c_dfDIKeyboard );
    if( FAILED(hr) ) 
        return hr;

    // Set the cooperativity level to let DirectInput know how
    // this device should interact with the system and with other
    // DirectInput applications.
    hr = g_pKeyboard->SetCooperativeLevel( hWnd, 
                                     DISCL_NONEXCLUSIVE | DISCL_FOREGROUND );

    if( FAILED(hr) ) 
        return hr;


    hr = DirectInputCreate( (HINSTANCE)GetWindowLong( hWnd, GWL_HINSTANCE ),
			DIRECTINPUT_VERSION, &g_pMouseDI, NULL );
    if ( FAILED(hr) ) 
        return hr;

   // Obtain an interface to the system mouse device.
    hr = g_pMouseDI->CreateDevice( GUID_SysMouse, &g_pMouse, NULL );
    if ( FAILED(hr) ) 
        return hr;

    // Set the data format to "mouse format" - a predefined data format 
    //
    // A data format specifies which controls on a device we
    // are interested in, and how they should be reported.
    //
    // This tells DirectInput that we will be passing a
    // DIMOUSESTATE structure to IDirectInputDevice::GetDeviceState.
    hr = g_pMouse->SetDataFormat( &c_dfDIMouse );
    if ( FAILED(hr) ) 
        return hr;

    // Set the cooperativity level to let DirectInput know how
    // this device should interact with the system and with other
    // DirectInput applications.
    hr = g_pMouse->SetCooperativeLevel( hWnd, 
                                        DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);

    if( FAILED(hr) ) 
        return hr;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: SetAcquire()
// Desc: Acquire or unacquire the keyboard, depending on if the app is active
//       Input device must be acquired before the GetDeviceState is called
//-----------------------------------------------------------------------------
HRESULT SetAcquire( BOOL bActive )
{
    // Nothing to do if g_pKeyboard is NULL

    if( bActive ) 
    {
        // Acquire the input device 
			if( NULL != g_pKeyboard ){
				g_pKeyboard->Acquire();
			}

      if( NULL != g_pMouse ){
				g_pMouse->Acquire();
			}

    } 
    else 
    {
        // Unacquire the input device 
			if( NULL != g_pKeyboard ){
				g_pKeyboard->Unacquire();
			}

      if( NULL != g_pMouse ){
				g_pMouse->Unacquire();
			}
    }

    return S_OK;
}



DIMOUSESTATE g_dims;
//-----------------------------------------------------------------------------
// Name: HandleInput()
// Desc: Get the input device's state and display it.
//-----------------------------------------------------------------------------
HRESULT CMyApplication::GetUserInput( CRacer* pRacer )
{
	BYTE		diks[256];	 // DirectInput keyboard state buffer 
	HRESULT hr;
	DIMOUSESTATE dims;      // DirectInput mouse state structure
	FLOAT inputPitch, inputRoll;
	
	if( NULL == g_pKeyboard ){
		MyAlert( NULL, "Keyboard device not initialized" );
		return E_FAIL;
	}
		
	hr = DIERR_INPUTLOST;
		
	// If input is lost then acquire and keep trying 
	while( DIERR_INPUTLOST == hr ) {
		// Get the input's device state, and put the state in dims
		hr = g_pKeyboard->GetDeviceState( sizeof(diks), &diks );
			
		if( hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED ){
			// DirectInput is telling us that the input stream has been
			// interrupted.  We aren't tracking any state between polls, so
			// we don't have any special reset that needs to be done.
			// We just re-acquire and try again.
			hr = g_pKeyboard->Acquire();
		}
	}
		
	if( FAILED(hr) ){  
		return hr;
	}
		

	if( g_pMouse ){
			
		hr = DIERR_INPUTLOST;
			
		// if input is lost then acquire and keep trying 
		while ( DIERR_INPUTLOST == hr ) {
			// get the input's device state, and put the state in dims
			hr = g_pMouse->GetDeviceState( sizeof(DIMOUSESTATE), &dims );
				
			if ( hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED ){
				// DirectInput is telling us that the input stream has
				// been interrupted.  We aren't tracking any state
				// between polls, so we don't have any special reset
				// that needs to be done.  We just re-acquire and
				// try again.
				hr = g_pMouse->Acquire();
				if ( FAILED(hr) )  
					return hr;
			}
		}
			
		if ( FAILED(hr) ){
			return hr;
		}
	}
	else{
		//ZeroMemory( &dims, sizeof(DIMOUSESTATE) );
		MyAlert( NULL, "GetUserInput() > Mouse device not initialized" );
		return E_FAIL;
	}


	if( pRacer != NULL ){
					
		ZeroMemory( &pRacer->input, sizeof(RACERINPUT) );

		//pRacer->input.turn = -(FLOAT)dims.lX * INPUT_MOUSESENSITIVITY_X;
		pRacer->input.lookPitch = -(FLOAT)dims.lY * INPUT_MOUSESENSITIVITY_Y;
		//pRacer->input.accel = (dims.rgbButtons[1] & 0x80);

		pRacer->input.turn += ( diks[ MYDIK_TURNLEFT ] & 0x80 ) ? 1.0f : 0.0f;
		pRacer->input.turn -= ( diks[ MYDIK_TURNRIGHT ] & 0x80) ? 1.0f : 0.0f;

		pRacer->input.accel = ( ( diks[ MYDIK_ACCEL ] & 0x80 ) | diks[ MYDIK_ACCEL_ALT ] & 0x80 );

		pRacer->input.pitch += ( diks[ MYDIK_PITCHDOWN ] & 0x80 ) ? 1.0f : 0.0f;
		pRacer->input.pitch -= ( diks[ MYDIK_PITCHUP ] & 0x80 ) ? 1.0f : 0.0f;

		if( pRacer->input.pitch != 0 ){
			playerPitchAssistLevel = 0.0f;
		}
		else if( playerPitchAssistLevel < 1.0f ){
			playerPitchAssistLevel += PLAYERPITCHASSIST_RECHARGERATE * deltaTime;
			if( playerPitchAssistLevel > 1.0f ){
				playerPitchAssistLevel = 1.0f;
			}
		}

		if( pRacer->input.accel && bPlayerPitchAssist && playerPitchAssistLevel ){
			GetAIPitchRollInput( pRacer, pRacer->yaw, PLAYERPITCHASSIST_INCLINATION );
			pRacer->input.roll = 0; // don't assist with roll
			pRacer->input.pitch *= playerPitchAssistLevel;
		}

		pRacer->input.roll += ( ( diks[ MYDIK_ROLLLEFT ] & 0x80 ) | ( diks[ MYDIK_ROLLLEFT_ALT ] & 0x80 ) )
			? 1.0f : 0.0f;
		pRacer->input.roll -= ( ( diks[ MYDIK_ROLLRIGHT ] & 0x80 ) | ( diks[ MYDIK_ROLLRIGHT_ALT ] & 0x80 ) )
			? 1.0f : 0.0f;

		//pRacer->input.pitch = min( 1.0f, pRacer->input.pitch + userPitch );
		//pRacer->input.roll = min( 1.0f, pRacer->input.roll + userRoll );
	}
	
//			g_dims = dims;


//			if( elapsedTime > 2.0 ){
//				DWORD blah=0;
//			}

/*
				dims.lX, dims.lY, dims.lZ,
                     (dims.rgbButtons[0] & 0x80) ? '0' : ' ',
                     (dims.rgbButtons[1] & 0x80) ? '1' : ' ',
                     (dims.rgbButtons[2] & 0x80) ? '2' : ' ',
                     (dims.rgbButtons[3] & 0x80) ? '3' : ' ');
*/
	return S_OK;
}




//-----------------------------------------------------------------------------
// Name: FreeDirectInput()
// Desc: Initialize the DirectInput variables.
//-----------------------------------------------------------------------------
HRESULT FreeDirectInput()
{
    // Unacquire and release any DirectInputDevice objects.
    if( g_pKeyboard ) 
    {
        // Unacquire the device one last time just in case 
        // the app tried to exit while the device is still acquired.
        g_pKeyboard->Unacquire();
        g_pKeyboard->Release();
        g_pKeyboard = NULL;
    }

    if( g_pMouse ) 
    {
        // Unacquire the device one last time just in case 
        // the app tried to exit while the device is still acquired.
        g_pMouse->Unacquire();

        g_pMouse->Release();
        g_pMouse = NULL;
    }

    // Release any DirectInput objects.
    if( g_pKeyboardDI ) 
    {
        g_pKeyboardDI->Release();
        g_pKeyboardDI = NULL;
    }

    // Release any DirectInput objects.
    if( g_pMouseDI ) 
    {
        g_pMouseDI->Release();
        g_pMouseDI = NULL;
    }

    return S_OK;
}


/*
VOID CMyApplication::HandleInput(){

	if( isRacing ){

	//////////

	}
}

*/
