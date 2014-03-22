

class CGameApplication : public CMyApplication{

	HRESULT Init();

	BOOL		HandleKeyInputEvent( WPARAM );

	HRESULT	Move();

	VOID		FinalCleanup();

public:
	CGameApplication();
};