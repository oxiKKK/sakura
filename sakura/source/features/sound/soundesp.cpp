#include "../../client.h"

std::deque<player_sound_no_index_t> Sound_No_Index;
std::deque<player_sound_index_t> Sound_Index;

float Sakura::Esp::realDynamicSoundVolume;

void Sakura::Esp::ChangeDynamicSoundVolume(float newVolume)
{
	realDynamicSoundVolume = newVolume;
}

void Sakura::Esp::DynamicSound(int entid, DWORD entchannel, char* szSoundFile, float* fOrigin, float fVolume, float fAttenuation, int iTimeOff, int iPitch)
{
	realDynamicSoundVolume = fVolume;

	if (!szSoundFile)
		return;

	if (!fOrigin)
		return;

	if (cvar.misc_fire_sounds && strstr(szSoundFile, /*weapons*/XorStr<0x38, 8, 0x1B96AECA>("\x4F\x5C\x5B\x4B\x53\x53\x4D" + 0x1B96AECA).s))
	{
		realDynamicSoundVolume = cvar.misc_fire_sounds_volume / 100.f;
		iPitch = cvar.misc_fire_sounds_pitch;
	}

	if (strstr(szSoundFile, "player") &&
		!strstr(szSoundFile, "pl_shell") &&
		!strstr(szSoundFile, "ric") &&
		!strstr(szSoundFile, "glass") &&
		!strstr(szSoundFile, "debris"))
	{
		if (entid > 0 && entid < 33)
		{
			int damage = 0;

			if (strstr(szSoundFile, "bhit_helmet"))
				damage = g_Engine.pfnRandomLong(65, 70);

			if (strstr(szSoundFile, "bhit_kevlar"))
				damage = g_Engine.pfnRandomLong(15, 20);

			if (strstr(szSoundFile, "bhit_flesh"))
				damage = g_Engine.pfnRandomLong(25, 30);

			if (strstr(szSoundFile, "headshot"))
				damage = g_Engine.pfnRandomLong(75, 80);

			if (damage <= g_Player[entid].iHealth)
				g_Player[entid].iHealth -= damage;

			if (strstr(szSoundFile, "die") || strstr(szSoundFile, "death"))
				g_Player[entid].iHealth = 100;

			if (cvar.visual_sound_steps)
			{
				player_sound_index_t sound_index;
				sound_index.index = entid;
				sound_index.origin = fOrigin;
				sound_index.timestamp = GetTickCount();
				Sound_Index.push_back(sound_index);
			}
		}
		else if (cvar.visual_sound_steps)
		{
			player_sound_no_index_t sound_no_index;
			sound_no_index.origin = fOrigin;
			sound_no_index.timestamp = GetTickCount();
			Sound_No_Index.push_back(sound_no_index);
		}
	}

	for (size_t i = 0; i < Sakura::Lua::scripts.size(); ++i)
	{
		auto& script = Sakura::Lua::scripts[i];

		if (!script.HasCallback(Sakura::Lua::SAKURA_CALLBACK_TYPE::SAKURA_CALLBACK_AT_DYNAMICSOUND))
			continue;

		auto& callbacks = script.GetCallbacks(Sakura::Lua::SAKURA_CALLBACK_TYPE::SAKURA_CALLBACK_AT_DYNAMICSOUND);
		for (const auto& callback : callbacks)
		{
			try
			{
				Vector origin;
				origin.x = fOrigin[0];
				origin.x = fOrigin[1];
				origin.x = fOrigin[2];

				callback(entid, std::string(szSoundFile), realDynamicSoundVolume, origin);
			}
			catch (luabridge::LuaException const& error)
			{
				if (script.GetState())
				{
					Sakura::Lua::Error(/*Error has occured in the lua "Dynamic Sound" script: %s*/XorStr<0x4F, 56, 0x58589174>("\x0A\x22\x23\x3D\x21\x74\x3D\x37\x24\x78\x36\x39\x38\x29\x2F\x3B\x3B\x40\x08\x0C\x43\x10\x0D\x03\x47\x04\x1C\x0B\x4B\x4E\x29\x17\x01\x11\x1C\x1B\x10\x54\x26\x19\x02\x16\x1D\x58\x5B\x0F\x1E\x0C\x16\xF0\xF5\xB8\xA3\xA1\xF6" + 0x58589174).s, error.what());
					script.RemoveAllCallbacks();
				}
			}
		}
	}

	PreS_DynamicSound_s(entid, entchannel, szSoundFile, fOrigin, realDynamicSoundVolume, fAttenuation, iTimeOff, iPitch);
}

void Sakura::Esp::DrawSoundIndex()
{
	if (!cvar.visual_sound_steps)
		return;

	for (player_sound_index_t sound_index : Sound_Index)
	{
		cl_entity_s* ent = g_Engine.GetEntityByIndex(sound_index.index);

		if (!ent)
			continue;

		if (cvar.visual_idhook_only && IdHook::FirstKillPlayer[sound_index.index] != 1)
			continue;

		if (!cvar.visual_visual_team && g_Player[sound_index.index].iTeam == g_Local.iTeam || ent->index == pmove->player_index + 1)
			continue;

		ImRGBA soundEspColor = Sakura::Colors::GetCustomizedTeamColor(sound_index.index, cvar.visual_sound_steps_color_tt, cvar.visual_sound_steps_color_ct);

		float step = M_PI * 2.0f / cvar.visual_sound_steps_segments;
		float radius = cvar.visual_sound_steps_radius * (1200 - (GetTickCount() - sound_index.timestamp)) / 1200;
		Vector position = Vector(sound_index.origin.x, sound_index.origin.y, sound_index.origin.z - 36);
		for (float i = 0; i < (IM_PI * 2.0f); i += step)
		{
			Vector vPointStart(radius * cosf(i) + position.x, radius * sinf(i) + position.y, position.z);
			Vector vPointEnd(radius * cosf(i + step) + position.x, radius * sinf(i + step) + position.y, position.z);
			float vStart[2], vEnd[2];

			if (WorldToScreen(vPointStart, vStart) && WorldToScreen(vPointEnd, vEnd))
			{
				ImGui::GetCurrentWindow()->DrawList->AddLine({ IM_ROUND(vStart[0]), IM_ROUND(vStart[1]) }, { IM_ROUND(vEnd[0]), IM_ROUND(vEnd[1]) }, ImColor(soundEspColor.r, soundEspColor.g, soundEspColor.b, soundEspColor.a), cvar.visual_sound_steps_segment_thickness);
			}
		}

		if (ent->curstate.messagenum == g_Engine.GetEntityByIndex(pmove->player_index + 1)->curstate.messagenum)
			continue;

		if (GetTickCount() - sound_index.timestamp > 300)
			continue;

		Vector vPointTop = Vector(sound_index.origin.x, sound_index.origin.y, sound_index.origin.z + 10);
		Vector vPointBot = Vector(sound_index.origin.x, sound_index.origin.y, sound_index.origin.z - 10);

		float vTop[2], vBot[2];
		if (WorldToScreen(vPointTop, vTop) && WorldToScreen(vPointBot, vBot))
		{
			float h = vTop[1] - vBot[1]; 
			float w = h;
			float x = vTop[0] - (w / 2);
			float y = vTop[1];

			Player::DrawBox(x, y, w, h, soundEspColor);
			Player::DrawHealth(sound_index.index, x, y, h);
			Player::DrawName(sound_index.index, x + (w / 2), y);
			Player::DrawVip(sound_index.index, x + w, y);
		}
	}
}

void Sakura::Esp::DrawSoundNoIndex()
{
	if (!cvar.visual_sound_steps)
		return;

	ImRGBA soundStepsColor = Sakura::Colors::GetCustomizedColor(cvar.visual_sound_steps_color);

	for (player_sound_no_index_t sound_no_index : Sound_No_Index)
	{
		float step = IM_PI * 2.0f / cvar.visual_sound_steps_segments;
		float radius = cvar.visual_sound_steps_radius * (1200 - (GetTickCount() - sound_no_index.timestamp)) / 1200;
		Vector position = Vector(sound_no_index.origin.x, sound_no_index.origin.y, sound_no_index.origin.z - 36);
		for (float i = 0; i < (IM_PI * 2.0f); i += step)
		{
			Vector vPointStart(radius * cosf(i) + position.x, radius * sinf(i) + position.y, position.z);
			Vector vPointEnd(radius * cosf(i + step) + position.x, radius * sinf(i + step) + position.y, position.z);
			float vStart[2], vEnd[2];
			if (WorldToScreen(vPointStart, vStart) && WorldToScreen(vPointEnd, vEnd))
				ImGui::GetCurrentWindow()->DrawList->AddLine({ IM_ROUND(vStart[0]), IM_ROUND(vStart[1]) }, { IM_ROUND(vEnd[0]), IM_ROUND(vEnd[1]) }, ImColor(soundStepsColor.r, soundStepsColor.g, soundStepsColor.b, soundStepsColor.a), cvar.visual_sound_steps_segment_thickness);
		}

		if (GetTickCount() - sound_no_index.timestamp > 300)
			continue;

		Vector vPointTop = Vector(sound_no_index.origin.x, sound_no_index.origin.y, sound_no_index.origin.z + 10);
		Vector vPointBot = Vector(sound_no_index.origin.x, sound_no_index.origin.y, sound_no_index.origin.z - 10);

		float vTop[2], vBot[2];
		if (WorldToScreen(vPointTop, vTop) && WorldToScreen(vPointBot, vBot))
		{
			float h = vTop[1] - vBot[1];
			float w = h;
			float x = vTop[0] - (w / 2);
			float y = vTop[1];

			Player::DrawBox(x, y, w, h, soundStepsColor);
		}
	}
}