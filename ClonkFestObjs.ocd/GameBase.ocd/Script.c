/*-- 
	Party game base
	Author: Sven2
	
	Derive party game objects from this!
--*/

#include Library_Goal // don't really need the IsFulfilled part, but we want the goal symbol

// Helper functions:
// GetGamePlayers() - return array of player numbers of players who play in the game
// GetLivingGamePlayers() - return array of players that aren't ghosts
// GhostPlayer(int player) - remove crew of player and turn cursor into a ghost

// SetGameWinners(array winners) - store winners of this game
// FinishGame() - mark game as ended and start next

static g_game; // Singleton variable set to instance of this

local startup_wait_time; // countdown

/* To be overloaded by games */

// Your name here
func GetGameAuthor() { return "Anonymous"; }

// Player restrictions. Return nil for no limit.
func GetGameMinPlayers() { return 2; }
func GetGameMaxPlayers() { return nil; }

// Section of landscape to be used for this game
// Multiple games may share the same map and a map may have multiple maps.
func GetGameSection() { return "TODO"; }

// Max player zoom range. Set as value and max value at start of game
func GetGameMaxZoomRange() { return {x=LandscapeWidth()/2, y=LandscapeHeight()/2}; }

// Player Clonk start position. Called once for each player. Called again for ghosts if there is no crew object left
func GetGameStartPos(int player) { return {x=LandscapeWidth()/2+Random(101)-50, y=LandscapeHeight()/2}; }

// Return true if the player may not scroll around
func GetGameViewLock() { return true; }

// Default Clonk type. Return nil if no Clonks should be created.
func GetGameClonkType() { return Clonk; }

// Max energy of Clonks
func GetGameClonkMaxEnergy() { return 10; }

// Seconds to wait btween InitGame and StartGame
func GetGameStartupTime() { return 3; }

// Set this to number of frames if the game should end automatically after a set time
func GetGameTimeLimit() { return nil; }

// Return true if game should end automatically if only one player is left alive
func IsGameLastManStanding() { return false; }

// Callback function to be overloaded for custom game initialization
// This is called after section load and all generic game initialization (Clonk placement, player properties set, etc.)
// players is an array of player numbers of players that participate. Do not assume that all players take part.
// Game should be initialized in a paused state if GetGameStartupTime() is used
func InitGame(array players) { return true; }

// To be overloaded after GetGameStartupTime() has passed.
func StartGame(array players) { return true; }

// Callback function when game is finished
// Call stuff like SetGameWinners() here.
func OnGameFinished()
{
	// By default, all players that aren't ghosts win the game
	SetGameWinners(GetLivingGamePlayers());
	return true;
}

// Static (definition) call done once a party has been started. Use to init static variables.
func InitFest() { return true; }

// Status
func IsGame() { return true; }


/* Initialization */

func Initialize()
{
	// Singleton init
	g_game = this;
	// Status
	this.Visibility = VIS_None;
	// Goal base init
	return _inherited(...);
}

func InitGameBase()
{
	// Called by scenario: Start the game!
	// Load specified section - only some internal objects will survive!
	if (!GameCall("IsTemplate")) LoadScenarioSectionClonkFest(GetGameSection(), this);
	// Reset landscape settings
	ResetGamma(3);
	SetSkyAdjust(0xffffffff); SetMatAdjust(0xffffffff);
	SetGravity(20);
	// Default time object
	//var time = CreateObject(Environment_Time);
	//time->SetTime(60*20);
	//time->SetCycleSpeed(20);
	// Init players
	EnableAllControls();
	var players = GetGamePlayers();
	var clonk_type = GetGameClonkType();
	for (var plr in players)
	{
		// Viewport settings
		var zoom_range = GetGameMaxZoomRange();
		SetPlayerZoomByViewRange(plr, zoom_range.x, zoom_range.y, PLRZOOM_LimitMax|PLRZOOM_Direct);	
		SetPlayerZoomByViewRange(plr, 120, 90, PLRZOOM_LimitMin);
		SetPlayerViewLock(plr, GetGameViewLock());
		// Properties
		SetWealth(plr, 0);
		// Initial Clonk init
		if (clonk_type)
		{
			var pos = GetGameStartPos(plr);
			var clonk = CreateObject(clonk_type, pos.x,pos.y, plr);
			if (clonk)
			{
				clonk->MakeCrewMember(plr);
				clonk.MaxEnergy = GetGameClonkMaxEnergy()*1000;
				clonk->DoEnergy(clonk.MaxEnergy/1000);
				SetCursor(plr, clonk);
			}
		}
	}
	// Info message
	NotifyHUD();
	ShowEntryMsg(NO_OWNER);
	// Custom game init
	InitGame(players);
	// Schedule game start
	startup_wait_time = GetGameStartupTime();
	if (startup_wait_time)
	{
		AddTimer("GameStartCountdown", 36);
		GameStartCountdown();
	}
	else
		StartGame(players);
	return true;
}

func GameStartCountdown()
{
	if (startup_wait_time)
	{
		Sound("Click", true);
		CustomMessage(Format("@<c ff0000>%d...</c>", startup_wait_time));
		--startup_wait_time;
	}
	else
	{
		Sound("BalloonPop", true);
		CustomMessage("<c ff00>$MsgGo$</c>");
		RemoveTimer("GameStartCountdown");
		StartGame(GetGamePlayers());
	}
	return true;
}

func InitializePlayer(int plr)
{
	// New players: Default join as ghost
	GhostPlayer(plr);
	return _inherited(plr, ...);
}

func RelaunchPlayer(int plr)
{
	//Log("Relaunch %d", plr);
	// Ignore during section loading
	if (g_loading_section) return;
	// Default player elimination: Relaunch as ghost
	GhostPlayer(plr);
	// If this is last man standing, check game end condition
	if (IsGameLastManStanding() && GetLength(GetLivingGamePlayers())<=1) FinishGame();
	return true;
}


/* End */

local game_finished, game_winners;

func SetGameWinners(array winners, bool freeze_game_on_winners)
{
	if (GetType(winners) == C4V_Int)
		game_winners = [winners];
	else if (GetType(winners) == C4V_Nil)
		game_winners = [];
	else if (GetType(winners) == C4V_Array)
		game_winners = winners;
	else
		FatalError(Format("SetGameWinners: Invalid parameter type. int, array or nil expected. Got value \"%v\".", winners));
	if (freeze_game_on_winners) FocusWinners(game_winners);
	return true;
	
}

func FinishGame()
{
	// Finish only once
	if (game_finished) return;
	game_finished = true;
	// Internal evaluation; determine winner(s)
	var finish_delay = OnGameFinished();
	// Callback to scenario!
	if (!game_winners) game_winners = [];
	var game_winners_s, i, n=GetLength(game_winners);
	var sep = ",";
	if (!n)
		game_winners_s = "[]";
	else
	{
		game_winners_s = "[";
		for (var winner in game_winners)
		{
			if (++i==n) sep = "]";
			game_winners_s = Format("%s%d%s", game_winners_s, winner, sep);
		}
	}
	// schedule in global context so it's save to do LoadScenarioSection
	Schedule(nil, Format("GameCall(\"OnGameFinished\", %s)", game_winners_s), Max(finish_delay,1));
	return true;
}

/* Helper functions */

func GetGamePlayers()
{
	// For now, all players take part
	var cnt = GetPlayerCount(C4PT_User);
	var players = CreateArray(cnt), i;
	for (var iplr = 0; iplr<cnt; ++iplr) players[i++] = GetPlayerByIndex(iplr,C4PT_User);
	return players;
}

func GetLivingGamePlayers()
{
	// For now, all players take part
	// Return players that aren't ghosted.
	var cnt = GetPlayerCount(C4PT_User);
	var players = CreateArray(cnt), i;
	for (var iplr = 0; iplr<cnt; ++iplr)
	{
		var plr = GetPlayerByIndex(iplr,C4PT_User);
		if (GetCrew(plr) && !GetCrew(plr)->~IsGhost()) players[i++] = plr;
	}
	if (i<cnt) SetLength(players, i);
	return players;
}


/* Goal interface */

// Prevent GameOver
public func IsFulfilled() { return false; }

func ShowEntryMsg(int byplr)
{
	CustomMessage(Format("<c ffff00>%s:</c> %s <c afafaf>    (by %s)</c>", this.Name, this.Description, GetGameAuthor()), nil, byplr, 5,10, 0xffffff, GUI_MenuDeco, this, MSG_HCenter | MSG_WidthRel | MSG_XRel | MSG_YRel, 80);
	return true;
}

func Activate(int byplr) { return ShowEntryMsg(byplr); }

func GetShortDescription(int plr) { return Name; }


/* Ghosts */

local GhostClonk;

func Definition()
{
	GhostClonk = { Prototype = Clonk,
		IsGhost = this.IsGame,
		Initialize = this.Ghost_Initialize,
		ContactIncinerate = 0,
		HasFeelings = true, // Ghosts have feelings, too!
		Visibility = VIS_Owner | VIS_God,
	};
	return true;
}

func Ghost_Initialize()
{
	// Ghosts don't die (again)
	DoEnergy(100);
	MakeInvincible();
	SetObjectBlitMode(1);
	SetClrModulation(0x80ffffff);
	if (Clonk.Initialize) Call(Clonk.Initialize, ...);
	return true;
}

func GhostPlayer(int plr)
{
	// Determine ghost position
	var ghost_pos;
	var last_crew = GetCursor(plr);
	if (!last_crew) last_crew = GetCrew(plr);
	if (last_crew) ghost_pos = [BoundBy(last_crew->GetX(),5,LandscapeWidth()-4), BoundBy(last_crew->GetDefBottom(),20,LandscapeHeight()-21)]; else ghost_pos = GetGameStartPos(plr);
	// Make sure player has no regular Clonks left
	for (var i = GetCrewCount(plr); i>=0; --i) if (last_crew=GetCrew(plr, i)) if (last_crew->GetAlive()) last_crew->Kill();
	// Create ghost!
	var ghost = CreateObject(GhostClonk, ghost_pos.x, ghost_pos.y, plr);
	if (ghost)
	{
		ghost->MakeCrewMember(plr);
		SetCursor(plr, ghost);
	}
	return true;
}


/* End sequences */

func FocusWinners(array winners)
{
	var n_winners = GetLength(winners);
	for(var plr in GetGamePlayers())
	{
		//Focus their window on the winning clonk
		if (n_winners && GetIndexOf(winners, plr)<0)
		{
			var target = GetCursor(winners[Random(n_winners)]);
			SetPlrView(plr, target);
			ScheduleCall(target, Global.SetPlrView,1,3,plr,target);
		}
		SetPlayerZoomByViewRange(plr, nil, 170, PLRZOOM_LimitMin|PLRZOOM_LimitMax);	//Zoom in
		for (var i=0; i<GetCrewCount(plr); ++i)
		{
			var crew = GetCrew(plr, i);
			crew->MakeInvincible();
			crew->SetCategory(1);
			crew->SetObjectLayer(crew);
			crew->SetCrewEnabled(false);
		}
	}
	return true;
}


/* Definition */

local Name = "$Name$";
local Description = "$Description$";