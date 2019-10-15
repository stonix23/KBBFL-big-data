
#include <string>
#include <unordered_map>

#include <vector>

#define YEAR_0   2013
#define YEAR_N   2018

enum Position { QB, RB, WR, TE, PK, DEF, MAX };

const char* Position2str[] = { "QB", "RB", "WR", "TE", "PK", "Def" };

unordered_map<string, Position> Str2Position({
    {"QB", Position::QB},
    {"RB", Position::RB},
    {"WR", Position::WR},
    {"TE", Position::TE},
    {"PK", Position::PK},
    {"Def", Position::DEF}
    });

Position starterPositionMap[8] = { QB, RB, RB, WR, WR, TE, PK, DEF };

inline Position& operator++(Position& p, int)  // <--- note -- must be a reference
{
    const int i = static_cast<int>(p);
    p = static_cast<Position>(min((i + 1), Position::MAX));
    return p;
}


struct PlayerScoringHistory
{
    DWORD points;
    DWORD starts;
    BOOL years[1 + YEAR_N - YEAR_0];

    string printYears()
    {
        string yearStr;
        int startYear = -1;

        for (int y = YEAR_0; y <= YEAR_N; y++)
        {
            if (this->years[y - YEAR_0])
            {
                if (startYear == -1)
                    startYear = y;
            }
            else
            {
                if (startYear != -1)
                {
                    if (!yearStr.empty())
                        yearStr.append(", ");

                    yearStr.append(to_string(startYear));

                    if (y > (startYear + 1))
                    {
                        yearStr.append("-" + to_string(y - 1));
                    }

                    startYear = -1;
                }
            }

        }

        if (startYear != -1)
        {
            if (!yearStr.empty())
                yearStr.append(", ");

            yearStr.append(to_string(startYear));

            if (YEAR_N > startYear)
            {
                yearStr.append("-" + to_string(YEAR_N));
            }

        }

        return yearStr;

    }
};

struct franchiseData
{
    string franchiseName;

    unordered_map<DWORD, PlayerScoringHistory> playerStarts;

    PlayerScoringHistory teamScoring;
    vector<BYTE> vectorScores;
    float stdDevScores;

    PlayerScoringHistory oppScoring;

    DWORD teamWins;
    DWORD teamLosses;
    DWORD teamTies;

    DWORD kickerLeadingScorer;
    tuple<DWORD, DWORD, DWORD, BYTE> bestKickerScore;   // <maxScore, maxPlayerID, year, week>

    multimap<DWORD, tuple<DWORD, DWORD, BYTE>> leadingBenchScorers;     // multimap<maxScore, tuple<maxPlayerID, year, week>>

    // Positional scoring by year
    PlayerScoringHistory positionalScoring[Position::MAX][1 + YEAR_N - YEAR_0];
    PlayerScoringHistory positionalScoringTotal[Position::MAX];

    pair<DWORD, float> highestAvg[Position::MAX];
    pair<DWORD, DWORD> highestTotal[Position::MAX];
    pair<DWORD, DWORD> mostStarts[Position::MAX];

    DWORD numStarts[Position::MAX];
    DWORD playersUsed[Position::MAX];
};

#pragma once
