/*==============================================================================
  Created on: 01/13/2005
  This is a commandlet for tweaking the player skill equation.

  Author: Ron Prestenback
  © 2004, Epic Games, Inc. All Rights Reserved
==============================================================================*/

class PlayerSkillCommandlet extends Commandlet;

var	float	TeamWinPercentage;
var float	PersonalWinPercentage;
var	float	KillPercentage;
var	float	TotalPercentage;

var	float	TeamWins, TeamGames,
			DMWins, DMGames,
			Kills, Deaths, KillingSprees;

var int		ExperiencePoints;

enum EPlayerRating
{
	Novice,
	Experienced,
	Adept,
	Masterful,
	Godlike,
};

var	EPlayerRating	PlayerSkill;

const COLUMN_WIDTH = 9;
const RANDOM_COLPADDING = 15;
const RANDOM_VALUE_WIDTH = 8;
const RANDOM_PERC_WIDTH = 4;

event int Main( string Parms )
{
	local int pos;
	local string Matches, Samples;
	local array<string> Groups;

	if ( Left(Parms, 6) ~= "random" )
	{
		Parms = Mid(Parms, 6);
		if ( Left(Parms, 1) != "(" )
		{
			ShowUsage();
			return 1;
		}

		Parms = Mid(Parms, 1);
		pos = InStr(Parms, ")");
		if ( pos == -1 )
		{
			ShowUsage();
			return 1;
		}
		Parms = Left(Parms, pos);

		if ( !Divide(Parms, ",", Matches, Samples) )
		{
			ShowUsage();
			return 1;
		}

		if ( Matches == "" || Samples == "" )
		{
			ShowUsage();
			return 1;
		}

		GenerateRandomData(int(Matches), int(Samples));
		return 0;
	}

	Split(Parms, Groups, " ");
	if ( Groups.Length != 3 )
	{
		ShowUsage();
		return 1;
	}

	if ( !SetTeamPercentage(Groups[0]) )
	{
		ShowUsage();
		return 1;
	}

	if ( !SetDMPercentage(Groups[1]) )
	{
		ShowUsage();
		return 1;
	}

	if ( !SetKillPercentage(Groups[2]) )
	{
		ShowUsage();
		return 1;
	}

	CalulateXP();
	CalculateTotalPercentage();

	CalculateRating();

	ShowResults();
	return 0;
}

final function bool SetTeamPercentage( string TeamRatio )
{
	local string WinString, GameString;

	if ( !Divide(TeamRatio, "/", WinString, GameString) )
		return false;

	if ( WinString == "" )
		return false;
	TeamWins = float(WinString);
	if ( GameString == "" )
		return false;
	TeamGames = float(GameString);
	if ( TeamGames == 0 )
		return false;
	TeamWinPercentage = TeamWins / TeamGames;

	return true;
}

final function bool SetDMPercentage( string DMRatio )
{
	local string WinString, GameString;

	if ( !Divide(DMRatio, "/", WinString, GameString) )
		return false;

	if ( WinString == "" )
		return false;
	DMWins = float(WinString);
	if ( GameString == "" )
		return false;

	DMGames = float(GameString);
	if ( DMGames == 0 )
		return false;

	PersonalWinPercentage = DMWins / DMGames;

	return true;
}

final function bool SetKillPercentage( string KillRatio )
{
	local array<string> Parts;

	Split(KillRatio, Parts, "/");
	if ( Parts.Length != 3 )
		return false;

	if ( Parts[0] == "" )
		return false;
	Kills = float(Parts[0]);

	if ( Parts[1] == "" )
		return false;
	Deaths = float(Parts[1]);

	if ( Parts[2] == "" )
		return false;
	KillingSprees = float(Parts[2]);

	if ( Kills + Deaths == 0 )
		return false;

	KillPercentage = Kills / (Kills + Deaths);

	return true;
}

final function CalulateXP()
{
	ExperiencePoints = TeamGames + DMGames + KillingSprees;
}

final function CalculateTotalPercentage()
{
	TotalPercentage =
		TeamWinPercentage +
		PersonalWinPercentage * 4 +
		KillPercentage * 2;
}

final function CalculateRating()
{
	PlayerSkill = Novice;
	if ( ExperiencePoints >= 10 && (TotalPercentage > 1.0 || ExperiencePoints > 20) )
	{
		PlayerSkill = Experienced;
		if ( ExperiencePoints >= 25 && (TotalPercentage > 2.0 || ExperiencePoints > 100) )
		{
			PlayerSkill = Adept;
			if ( ExperiencePoints >= 50 && TotalPercentage > 3.0 )
			{
				PlayerSkill = Masterful;
				if ( ExperiencePoints >= 100 && TotalPercentage > 4.0 )
				{
					PlayerSkill = Godlike;
				}
			}
		}
	}
}

final function WriteHeader()
{
	local string Header;

	log("");
	Header = PadString("Exp. Points", COLUMN_WIDTH);
	Header @= PadString("Personal %", COLUMN_WIDTH);
	Header @= PadString("Team %", COLUMN_WIDTH);
	Header @= PadString("Kill %", COLUMN_WIDTH);
	Header @= PadString("Total %", COLUMN_WIDTH);
	Header @= "Rating";
	log(Header);
}

final function ShowResults()
{
	local string Result;

	WriteHeader();
	log("");
	Result $= PadString(ExperiencePoints, COLUMN_WIDTH," ",true);
	Result @= PadString(PersonalWinPercentage * 100, COLUMN_WIDTH," ",true);
	Result @= PadString(TeamWinPercentage * 100, COLUMN_WIDTH," ",true);
	Result @= PadString(KillPercentage * 100, COLUMN_WIDTH," ",true);
	Result @= PadString(TotalPercentage, COLUMN_WIDTH," ",true);
	Result @= PadString(GetEnum(enum'EPlayerRating', PlayerSkill), COLUMN_WIDTH);
	log(Result);
}

final function GenerateRandomData( int Matches, int Samples )
{
	local int W, T, TW, TT, K, D, KS;

	WriteRandomHeader(Matches);
	log("");
	while ( Samples-- > 0 )
	{
		// get random personal kills
		T = Max(1, Rand(Matches));
		W = float(Rand(T)) * FRand();
		SetDMPercentage( W $ "/" $ T );

		TW = Rand(Matches);
		TT = Max(1, RandRange(TW, Matches));
		SetTeamPercentage( TW $ "/" $ TT );

		K = Rand(Matches * Rand(30));
		D = Rand(Matches * Rand(30));
		KS = float(Matches) * FRand();
		SetKillPercentage( K $ "/" $ D $ "/" $ KS );

		CalulateXP();
		CalculateTotalPercentage();
		CalculateRating();
		WriteRandomResult(W,T,TW,TT,K,D,KS,Matches);
	}
}

final function WriteRandomHeader( int Matches )
{
	local string Header;

	log("");
	Header $= PadString("XP", RANDOM_VALUE_WIDTH, " ",true);

	Header $= PadString("Won/Played (DM)", RANDOM_COLPADDING + RANDOM_VALUE_WIDTH, " ",true);
	Header $= PadString("%", RANDOM_PERC_WIDTH, " ", true);

	Header $= PadString("Won/Played (TEAM)", RANDOM_COLPADDING + RANDOM_VALUE_WIDTH, " ",true);
	Header $= PadString("%", RANDOM_PERC_WIDTH, " ", true);

	Header $= PadString("Killed/Died/Sprees", RANDOM_COLPADDING + RANDOM_VALUE_WIDTH + float(RANDOM_VALUE_WIDTH) * 0.5, " ",true);
	Header $= PadString("%", RANDOM_PERC_WIDTH, " ", true);

	Header $= PadString("Total %", RANDOM_COLPADDING + RANDOM_PERC_WIDTH, " ",true);

	Header @= "Rating";
	log(Header);
}

final function WriteRandomResult( int W, int T, int TW, int TT, int K, int D, int KS, int Matches )
{
	local string Result;

	Result $= PadString(ExperiencePoints, RANDOM_VALUE_WIDTH, " ",true);
	Result $= PadString("", RANDOM_COLPADDING);

	Result $= PadString(W $ "/" $ T, RANDOM_VALUE_WIDTH, " ",true);
	Result $= PadString(int(PersonalWinPercentage * 100), RANDOM_PERC_WIDTH," ",true);
	Result $= PadString("", RANDOM_COLPADDING);

	Result $= PadString(TW $ "/" $ TT, RANDOM_VALUE_WIDTH," ",true);
	Result $= PadString(int(TeamWinPercentage * 100), RANDOM_PERC_WIDTH," ",true);
	Result $= PadString("", RANDOM_COLPADDING);

	Result $= PadString(K $ "/" $ D $ "/" $ KS, RANDOM_VALUE_WIDTH + float(RANDOM_VALUE_WIDTH) * 0.5," ",true);
	Result $= PadString(int(KillPercentage * 100), RANDOM_PERC_WIDTH," ",true);
	Result $= PadString("", RANDOM_COLPADDING);

	Result $= PadString(FloatToString(TotalPercentage, 2), RANDOM_PERC_WIDTH," ",true);
	Result @= GetEnum(enum'EPlayerRating', PlayerSkill);
	log(Result);
}

final function ShowUsage()
{
	local int i;

	log("Usage:");
	log("   ucc " $ HelpUsage);
	log("");
	log("Parameters:");
	for( i=0; i < ArrayCount(HelpParm) && HelpParm[i] != ""; i++ )
		log("   " $ PadString(HelpParm[i], 16) @ HelpDesc[i]);
}

/*==========================================================================================================
function	::Divide
Desc:		Create two strings by dividing a string based on a delimiter.
			(Turn "One sheep, two sheep" into "One sheep" and " two sheep")

input:		Text:
				the text to split up.

output:		LeftPart:
				the part of the original string that was on the left side of the delimiter,
				or the entire input string if the delimiter wasn't found.

			RightPart:
				the part of the original string that is on the right side of the delimiter, or empty
				if the delimiter isn't found.

return:		return true if the delimiter was found, false otherwise.

last modified by:	Ron Prestenback
==========================================================================================================*/

static final function bool Divide( string Text, string Delim, out string LeftPart, out string RightPart )
{
	local int i;

	RightPart = "";
	i = InStr(Text, Delim);
	if ( i == -1 )
	{
		LeftPart = Text;
		return false;
	}

	LeftPart = Left(Text, i);
	RightPart = Mid(Text, i + Len(Delim));
	return true;
}


DefaultProperties
{
	ShowBanner=false

/// Command name to show for "ucc help".
	HelpCmd="playerskill"

/// Command description to show for "ucc help".
	HelpOneLiner="Calculate player skill."

/// Usage template to show for "ucc help".
	HelpUsage="playerskill <random(matches,players) | TeamWins/TeamGames DMWins/DMGames Kills/Deaths/KillingSprees>"

/// Parameters and descriptions for "ucc help <this command>".
	HelpParm(0)="random(matches,players)"
	HelpDesc(0)="generate random match data and display the results"

	HelpParm(1)="TeamWins"
	HelpDesc(1)="number of times the player has won the match in a team game"

	HelpParm(2)="TeamGames"
	HelpDesc(2)="total number of team matches played"

	HelpParm(3)="DMWins"
	HelpDesc(3)="number of times the player has won the match in a non-team game"

	HelpParm(4)="DMGames"
	HelpDesc(4)="total number of non-team matches played"

	HelpParm(5)="Kills"
	HelpDesc(5)="number of kills scored by the player"

	HelpParm(6)="Deaths"
	HelpDesc(6)="number of times the player has died"

	HelpParm(7)="KillingSprees"
	HelpDesc(7)="number of killing sprees the player has achieved"
}
