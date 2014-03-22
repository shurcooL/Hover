// Hover
// Copyright 2000 Eric Undersander <eundersan@mail.utexas.edu>

#include "../common/common.h"
#include "hover.h"


//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: Entry point to the program. Initializes everything, and goes into a
//			 message-processing loop. Idle time is used to render the scene.
//-----------------------------------------------------------------------------
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
{
		CGameApplication gameApp;

		if( FAILED( gameApp.Create( hInst, strCmdLine ) ) )
				return 0;

		return gameApp.Run();
}
