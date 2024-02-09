#pragma once

#include "tier0/dbg.h"

class ICollideable;
class IServerEntity;
class IServerNetworkable;
class IServerUnknown;

struct edict_t;

class IChangeInfoAccessor
{
public:
	inline void SetChangeInfo( unsigned short info )
	{
		m_iChangeInfo = info;
	}

	inline void SetChangeInfoSerialNumber( unsigned short sn )
	{
		m_iChangeInfoSerialNumber = sn;
	}

	inline unsigned short	 GetChangeInfo() const
	{
		return m_iChangeInfo;
	}

	inline unsigned short	 GetChangeInfoSerialNumber() const
	{
		return m_iChangeInfoSerialNumber;
	}

private:
	unsigned short m_iChangeInfo;
	unsigned short m_iChangeInfoSerialNumber;
};

class CBaseEdict
{
public:

	// Returns an IServerEntity if FL_FULLEDICT is set or NULL if this 
	// is a lightweight networking entity.
	IServerEntity*			GetIServerEntity();
	const IServerEntity*	GetIServerEntity() const;

	IServerNetworkable*		GetNetworkable();
	IServerUnknown*			GetUnknown();

	// Set when initting an entity. If it's only a networkable, this is false.
	void				SetEdict( IServerUnknown *pUnk, bool bFullEdict );
		
	int					AreaNum() const;
	const char *		GetClassName() const;
		
	bool				IsFree() const;
	void				SetFree();
	void				ClearFree();

	bool				HasStateChanged() const;
	void				ClearStateChanged();
	void				StateChanged();
	void				StateChanged( unsigned short offset );

	void				ClearTransmitState();
		
	void SetChangeInfo( unsigned short info );
	void SetChangeInfoSerialNumber( unsigned short sn );
	unsigned short	 GetChangeInfo() const;
	unsigned short	 GetChangeInfoSerialNumber() const;

public:

	// NOTE: this is in the edict instead of being accessed by a virtual because the engine needs fast access to it.
	// NOTE: YOU CAN'T CHANGE THE LAYOUT OR SIZE OF CBASEEDICT AND REMAIN COMPATIBLE WITH HL2_VC6!!!!!
#ifdef _XBOX
	unsigned short m_fStateFlags;	
#else
	int	m_fStateFlags;	
#endif	

	// NOTE: this is in the edict instead of being accessed by a virtual because the engine needs fast access to it.
	int m_NetworkSerialNumber;	// Game DLL sets this when it gets a serial number for its EHANDLE.

	// NOTE: this is in the edict instead of being accessed by a virtual because the engine needs fast access to it.
	IServerNetworkable	*m_pNetworkable;

protected:
	IServerUnknown		*m_pUnk;		


public:

	IChangeInfoAccessor *GetChangeAccessor(); // The engine implements this and the game .dll implements as
	const IChangeInfoAccessor *GetChangeAccessor() const; // The engine implements this and the game .dll implements as
	// as callback through to the engine!!!

	// NOTE: YOU CAN'T CHANGE THE LAYOUT OR SIZE OF CBASEEDICT AND REMAIN COMPATIBLE WITH HL2_VC6!!!!!
	// This breaks HL2_VC6!!!!!
	// References a CEdictChangeInfo with a list of modified network props.
	//unsigned short m_iChangeInfo;
	//unsigned short m_iChangeInfoSerialNumber;

	friend void InitializeEntityDLLFields( edict_t *pEdict );
};


#define FL_EDICT_CHANGED	(1<<0)	// Game DLL sets this when the entity state changes
									// Mutually exclusive with FL_EDICT_PARTIAL_CHANGE.							
#define FL_EDICT_FREE		(1<<1)	// this edict if free for reuse
#define FL_EDICT_FULL		(1<<2)	// this is a full server entity

#define FL_EDICT_FULLCHECK	(0<<0)  // call ShouldTransmit() each time, this is a fake flag
#define FL_EDICT_ALWAYS		(1<<3)	// always transmit this entity
#define FL_EDICT_DONTSEND	(1<<4)	// don't transmit this entity
#define FL_EDICT_PVSCHECK	(1<<5)	// always transmit entity, but cull against PVS

// Used by local network backdoor.
#define FL_EDICT_PENDING_DORMANT_CHECK	(1<<6)

// This is always set at the same time EFL_DIRTY_PVS_INFORMATION is set, but it 
// gets cleared in a different place.
#define FL_EDICT_DIRTY_PVS_INFORMATION	(1<<7)

// This is used internally to edict_t to remember that it's carrying a 
// "full change list" - all its properties might have changed their value.
#define FL_FULL_EDICT_CHANGED			(1<<8)


// Max # of variable changes we'll track in an entity before we treat it
// like they all changed.
#define MAX_CHANGE_OFFSETS	19
#define MAX_EDICT_CHANGE_INFOS	100

//-----------------------------------------------------------------------------
// CBaseEdict inlines.
//-----------------------------------------------------------------------------
inline IServerEntity* CBaseEdict::GetIServerEntity()
{
	if ( m_fStateFlags & FL_EDICT_FULL )
		return (IServerEntity*)m_pUnk;
	else
		return 0;
}

inline bool CBaseEdict::IsFree() const
{
	return (m_fStateFlags & FL_EDICT_FREE) != 0;
}



inline bool	CBaseEdict::HasStateChanged() const
{
	return (m_fStateFlags & FL_EDICT_CHANGED) != 0;
}

inline void	CBaseEdict::ClearStateChanged()
{
	m_fStateFlags &= ~(FL_EDICT_CHANGED | FL_FULL_EDICT_CHANGED);
	SetChangeInfoSerialNumber( 0 );
}

inline void	CBaseEdict::StateChanged()
{
	// Note: this should only happen for properties in data tables that used some
	// kind of pointer dereference. If the data is directly offsetable 
	m_fStateFlags |= (FL_EDICT_CHANGED | FL_FULL_EDICT_CHANGED);
	SetChangeInfoSerialNumber( 0 );
}

inline void	CBaseEdict::StateChanged( unsigned short offset )
{
	if ( m_fStateFlags & FL_FULL_EDICT_CHANGED )
		return;

	Assert(false);
}

inline void CBaseEdict::SetFree()
{
	m_fStateFlags |= FL_EDICT_FREE;
}

inline void CBaseEdict::ClearFree()
{
	m_fStateFlags &= ~FL_EDICT_FREE;
}

inline void	CBaseEdict::ClearTransmitState()
{
	m_fStateFlags &= ~(FL_EDICT_ALWAYS|FL_EDICT_PVSCHECK|FL_EDICT_DONTSEND);
}

inline const IServerEntity* CBaseEdict::GetIServerEntity() const
{
	if ( m_fStateFlags & FL_EDICT_FULL )
		return (IServerEntity*)m_pUnk;
	else
		return 0;
}

inline IServerUnknown* CBaseEdict::GetUnknown()
{
	return m_pUnk;
}

inline IServerNetworkable* CBaseEdict::GetNetworkable()
{
	return m_pNetworkable;
}

inline void CBaseEdict::SetEdict( IServerUnknown *pUnk, bool bFullEdict )
{
	m_pUnk = pUnk;
	if ( (pUnk != NULL) && bFullEdict )
	{
		m_fStateFlags = FL_EDICT_FULL;
	}
	else
	{
		m_fStateFlags = 0;
	}
}

inline int CBaseEdict::AreaNum() const
{
	if ( !m_pUnk )
		return 0;
	Assert(false);
}

inline const char *	CBaseEdict::GetClassName() const
{
	if ( !m_pUnk )
		return "";
	Assert(false);
}

inline void CBaseEdict::SetChangeInfo( unsigned short info )
{
	GetChangeAccessor()->SetChangeInfo( info );
}

inline void CBaseEdict::SetChangeInfoSerialNumber( unsigned short sn )
{
	GetChangeAccessor()->SetChangeInfoSerialNumber( sn );
}

inline unsigned short CBaseEdict::GetChangeInfo() const
{
	return GetChangeAccessor()->GetChangeInfo();
}

inline unsigned short CBaseEdict::GetChangeInfoSerialNumber() const
{
	return GetChangeAccessor()->GetChangeInfoSerialNumber();
}


struct edict_t : public CBaseEdict
{
public:
	ICollideable *GetCollideable();

	// The server timestampe at which the edict was freed (so we can try to use other edicts before reallocating this one)
	float		freetime;	
};
