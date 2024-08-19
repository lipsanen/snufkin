#include "autojump.hpp"
#include "tier1/convar.h"
#include "../plugin.h"
#include "SDK/hl_movedata.h"

AutojumpFeature feat_autojump;
ConVar sn_autojump("sn_autojump", "0", FCVAR_ARCHIVE | FCVAR_DEMO | FCVAR_CHEAT | FCVAR_DEMO);

namespace patterns
{
	PATTERNS(
	    CheckJumpButton,
	    "5135",
	    "83 EC 1C 56 8B F1 8B 4E 04 80 B9 04 0A 00 00 00 74 0E 8B 76 08 83 4E 28 02 32 C0 5E 83 C4 1C C3 D9 EE D8 91 70 0D 00 00",
	    "4104",
	    "83 EC 1C 56 8B F1 8B 4E 08 80 B9 C4 09 00 00 00 74 0E 8B 76 04 83 4E 28 02 32 C0 5E 83 C4 1C C3 D9 EE D8 91 30 0D 00 00");
	PATTERNS(
	    CheckJumpButton_client,
	    "5135",
	    "83 EC 18 56 8B F1 8B 4E 04 80 B9 74 0F 00 00 00 74 0E 8B 76 08 83 4E 28 02 32 C0 5E 83 C4 18 C3 D9 EE D8 91 6C 10 00 00",
	    "4104",
	    "83 EC 18 56 8B F1 8B 4E 08 80 B9 3C 0F 00 00 00 74 0E 8B 76 04 83 4E 28 02 32 C0 5E 83 C4 18 C3 D9 EE D8 91 30 10 00 00");
} // namespace patterns

void AutojumpFeature::PreHook()
{
	ptrCheckJumpButton = (uintptr_t)ORIG_CheckJumpButton;
}

void AutojumpFeature::InitHooks()
{
	HOOK_FUNCTION(server, CheckJumpButton);
	HOOK_FUNCTION(client, CheckJumpButton_client);
}

bool AutojumpFeature::ShouldLoadFeature()
{
	return true;
}

void AutojumpFeature::LoadFeature()
{
	// Server-side CheckJumpButton
	if (ORIG_CheckJumpButton)
	{
		InitConcommandBase(sn_autojump);
		int ptnNumber = GetPatternIndex((void**)&ORIG_CheckJumpButton);
		switch (ptnNumber)
		{
		case 0:  // 5135
			off_mv_ptr = 2;
			break;

		case 1:  // 4104
			off_mv_ptr = 1;
			break;
		}
		if (off_mv_ptr == 1)
			off_player_ptr = 2;
		else if (off_mv_ptr == 2)
			off_player_ptr = 1;
	}
}

void AutojumpFeature::UnloadFeature()
{
	ptrCheckJumpButton = NULL;
}

static Vector old_vel(0.0f, 0.0f, 0.0f);

IMPL_HOOK_THISCALL(AutojumpFeature, bool, CheckJumpButton, void*)
{
	bool shouldAutoJump = CheckCheatVarBool(sn_autojump);

	const int IN_JUMP = (1 << 1);

	int* pM_nOldButtons = NULL;
	int origM_nOldButtons = 0;
	CHLMoveData* mv = (CHLMoveData*)(*((uintptr_t*)thisptr + feat_autojump.off_mv_ptr));

	if (shouldAutoJump)
	{
		pM_nOldButtons = &mv->m_nOldButtons;
		origM_nOldButtons = *pM_nOldButtons;

		if (!feat_autojump.cantJumpNextTime) // Do not do anything if we jumped on the previous tick.
		{
			*pM_nOldButtons &= ~IN_JUMP; // Reset the jump button state as if it wasn't pressed.
		}
	}

	feat_autojump.cantJumpNextTime = false;

	feat_autojump.insideCheckJumpButton = true;
	bool rv = feat_autojump.ORIG_CheckJumpButton(thisptr); // This function can only change the jump bit.
	feat_autojump.insideCheckJumpButton = false;

	if (shouldAutoJump)
	{
		if (!(*pM_nOldButtons & IN_JUMP)) // CheckJumpButton didn't change anything (we didn't jump).
		{
			*pM_nOldButtons = origM_nOldButtons; // Restore the old jump button state.
		}
	}

	if (rv)
	{
		feat_autojump.cantJumpNextTime = true; // Prevent consecutive jumps.
	}

	return rv;
}

IMPL_HOOK_THISCALL(AutojumpFeature, bool, CheckJumpButton_client, void*)
{
	/*
	Not sure if this gets called at all from the client dll, but
	I will just hook it in exactly the same way as the server one.
	*/
	const int IN_JUMP = (1 << 1);

	int* pM_nOldButtons = NULL;
	int origM_nOldButtons = 0;

	CHLMoveData* mv = (CHLMoveData*)(*((uintptr_t*)thisptr + feat_autojump.off_mv_ptr));
	if (CheckCheatVarBool(sn_autojump))
	{
		pM_nOldButtons = &mv->m_nOldButtons;
		origM_nOldButtons = *pM_nOldButtons;

		if (!feat_autojump.client_cantJumpNextTime) // Do not do anything if we jumped on the previous tick.
		{
			*pM_nOldButtons &= ~IN_JUMP; // Reset the jump button state as if it wasn't pressed.
		}
	}

	feat_autojump.client_cantJumpNextTime = false;

	feat_autojump.client_insideCheckJumpButton = true;
	bool rv = feat_autojump.ORIG_CheckJumpButton_client(thisptr); // This function can only change the jump bit.
	feat_autojump.client_insideCheckJumpButton = false;

	if (CheckCheatVarBool(sn_autojump))
	{
		if (!(*pM_nOldButtons & IN_JUMP)) // CheckJumpButton didn't change anything (we didn't jump).
		{
			*pM_nOldButtons = origM_nOldButtons; // Restore the old jump button state.
		}
	}

	if (rv)
	{
		// We jumped.
		feat_autojump.client_cantJumpNextTime = true; // Prevent consecutive jumps.
	}

	DevMsg("Engine call: [client dll] CheckJumpButton() => %s\n", (rv ? "true" : "false"));

	return rv;
}
