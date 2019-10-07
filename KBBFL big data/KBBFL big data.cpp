#include <Windows.h>

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <map>
#include <unordered_map>

#include "mflQuery.h"
#include "KBBFL big data.h"


void loadFranchiseInfo(franchiseData** franchiseDirectory, unordered_map<string, int> &franchiseMap);

void loadWeeklyResults(franchiseData** franchiseDirectory, unordered_map<DWORD, pair<string, Position>> &playerMap);

void generateTeamReport(franchiseData* franchiseDirectory, unordered_map<DWORD, pair<string, Position>> &playerMap);

void generateRankings(franchiseData** franchiseDirectory, unordered_map<DWORD, pair<string, Position>> &playerMap);


int main()
{

#if 0

    mflQuery* mflDataSession = new mflQuery(2019);

    mflDataSession->mflInit();

    string leagueInfoString;

    ofstream leagueFile("C:\\KBBFL big data\\KBBFL info.xml", std::ofstream::out);

    mflDataSession->mflRequest(leagueInfoString, 2018, L"league", L"");

    leagueFile << leagueInfoString;
    leagueFile.close();


    for (int year = 2013; year <= 2018; year++)
    {
        string resultsString, playersString;
        cout << "Procuring player list for KBBFL season " << year << endl;

        ofstream playersFile("C:\\KBBFL big data\\players_" + to_string(year) + ".xml", std::ofstream::out);

        mflDataSession->mflRequest(playersString, year, L"players", L"");
        playersFile << playersString;
        
        playersString.clear();
        playersFile.close();

        ofstream resultsFile("C:\\KBBFL big data\\weeklyResults_" + to_string(year) + ".xml", std::ofstream::out);

        cout << "Procuring results for KBBFL season " << year << endl;

        for (int week = 1; week <= 14; week++)
        {
            mflDataSession->mflRequest(resultsString, year, L"weeklyResults", L"&W=" + to_wstring(week));

            if (week == 1)
                resultsString.insert(resultsString.find("\n\n") + 2, "<seasonalResults>\n");
            else
                resultsString.erase(0, resultsString.find("\n") + 1);

            resultsFile << resultsString << endl;

            resultsString.clear();
        }

        resultsFile << "</seasonalResults>\n";
        resultsFile.close();

    }

    mflDataSession->~mflQuery();

#endif


    franchiseData* franchiseDirectory[17];
    unordered_map<string, int> franchiseMap;

    loadFranchiseInfo(franchiseDirectory, franchiseMap);


    unordered_map<DWORD, pair<string, Position>> playerMap;

    loadWeeklyResults(franchiseDirectory, playerMap);


    // Generate reports

    cout << endl;

    for (int teamID = 1; teamID <=16; teamID++)
    { 
        franchiseData* franchisePtr = franchiseDirectory[teamID];
    
        generateTeamReport(franchisePtr, playerMap);
    }

    generateRankings(franchiseDirectory, playerMap);

    cout << "End of report" << endl;
    cin.get();

    return 0;
}




void loadFranchiseInfo(franchiseData** franchiseDirectory, unordered_map<string, int> &franchiseMap)
{
    pugi::xml_parse_result resultXML;

    // Make map of franchise info

    pugi::xml_document leagueInfoXML;
    resultXML = leagueInfoXML.load_file(string("C:\\KBBFL big data\\KBBFL info.xml").data());

    pugi::xml_node franchiseNode = leagueInfoXML.first_child().find_child_by_attribute("count", "17");

    for (pugi::xml_node itNode = franchiseNode.first_child(); itNode; itNode = itNode.next_sibling())
    {
        int franchiseID = atoi(itNode.attribute("id").value());

        if (franchiseID <= 16)
        {
            franchiseDirectory[franchiseID] = new franchiseData();

            string franchiseName = itNode.attribute("name").value();
            franchiseDirectory[franchiseID]->franchiseName = franchiseName;

            franchiseMap[franchiseName] = franchiseID;
        }
    }

}



void loadWeeklyResults(franchiseData** franchiseDirectory, unordered_map<DWORD, pair<string, Position>> &playerMap)
{
    pugi::xml_parse_result resultXML;

    for (int year = YEAR_0; year <= YEAR_N; year++)
    {
        // Create player database for year
        pugi::xml_document playersXMLFile;
        resultXML = playersXMLFile.load_file(string("C:\\KBBFL big data\\players_" + to_string(year) + ".xml").data());

        for (pugi::xml_node itNode = playersXMLFile.first_child().first_child(); itNode; itNode = itNode.next_sibling())
        {
            string pos(itNode.attribute("position").value());

            int playerID = atoi(itNode.attribute("id").value());

            // Check if player is already added
            if (playerMap.find(playerID) != playerMap.end())
            {
                // Check if player switched positions
                if (playerMap[playerID].second != Str2Position[pos.data()])
                    playerMap[playerID] = make_pair(itNode.attribute("name").value(), Str2Position[pos.data()]);
            }
            else  // Check if player is at a relevant position
            if (Str2Position.find(pos.data()) != Str2Position.end())
            {
                playerMap[playerID] = make_pair(itNode.attribute("name").value(), Str2Position[pos.data()]);
            }

        }

        // Process weekly results for year
        pugi::xml_document resultsXMLFile;
        resultXML = resultsXMLFile.load_file(string("C:\\KBBFL big data\\weeklyResults_" + to_string(year) + ".xml").data());

        for (int week = 1; week <= 14; week++)
        {
            pugi::xml_node weeklyResultsNode = resultsXMLFile.first_child().find_child_by_attribute("week", to_string(week).data());

            for (pugi::xml_node matchupNode = weeklyResultsNode.first_child(); matchupNode; matchupNode = matchupNode.next_sibling())
            {
                BYTE matchupIDs[2];
                BYTE matchupScores[2];
                BYTE matchupIndex = 0;

                for (pugi::xml_node franchiseNode = matchupNode.first_child(); franchiseNode; franchiseNode = franchiseNode.next_sibling())
                {
                    BYTE teamID = atoi(franchiseNode.attribute("id").value());
                    BYTE teamScore = atoi(franchiseNode.attribute("score").value());

                    franchiseDirectory[teamID]->vectorScores.push_back(teamScore);
                    franchiseDirectory[teamID]->teamScoring.points += teamScore;
                    franchiseDirectory[teamID]->teamScoring.starts++;

                    matchupIDs[matchupIndex] = teamID;
                    matchupScores[matchupIndex] = teamScore;
                    matchupIndex++;

                    if ((franchiseNode.attribute("result").value()[0]) == 'W')
                        franchiseDirectory[teamID]->teamWins++;
                    else if ((franchiseNode.attribute("result").value()[0]) == 'L')
                        franchiseDirectory[teamID]->teamLosses++;
                    else
                        franchiseDirectory[teamID]->teamTies++;

                    DWORD maxScore = 0;
                    DWORD maxPlayerID = -1;
                    BOOL maxPointsOnBench = false;

                    for (pugi::xml_node playerNode = franchiseNode.first_child(); playerNode; playerNode = playerNode.next_sibling())
                    {
                        DWORD playerScore = atoi(playerNode.attribute("score").value());
                        DWORD playerID = atoi(playerNode.attribute("id").value());

                        if (playerScore > maxScore)
                        {
                            maxPlayerID = playerID;
                            maxScore = playerScore;

                            if (!strcmp(playerNode.attribute("status").value(), "nonstarter"))
                                maxPointsOnBench = true;
                        }

                        if (!strcmp(playerNode.attribute("status").value(), "starter"))
                        {
                            Position starterPos = playerMap[playerID].second;

                            if (!(franchiseDirectory[teamID]->playerStarts[playerID].starts++))
                                franchiseDirectory[teamID]->playersUsed[starterPos]++;

                            franchiseDirectory[teamID]->playerStarts[playerID].points += playerScore;
                            franchiseDirectory[teamID]->playerStarts[playerID].years[year - YEAR_0] = true;

                            PlayerScoringHistory* posScoringHist = &(franchiseDirectory[teamID]->positionalScoring[starterPos][year - YEAR_0]);

                            posScoringHist->points += playerScore;
                            posScoringHist->starts++;

                            franchiseDirectory[teamID]->numStarts[starterPos]++;
                        }

                    }

                    if (maxPointsOnBench)
                    {
                        if (maxPlayerID != -1)
                            franchiseDirectory[teamID]->leadingBenchScorers.insert({ maxScore, make_tuple(maxPlayerID, year, week) });
                    }

                }

                franchiseDirectory[matchupIDs[0]]->oppScoring.points += matchupScores[1];
                franchiseDirectory[matchupIDs[0]]->oppScoring.starts++;
                franchiseDirectory[matchupIDs[1]]->oppScoring.points += matchupScores[0];
                franchiseDirectory[matchupIDs[1]]->oppScoring.starts++;

            }
        }

    }

}


void generateTeamReport(franchiseData* franchisePtr, unordered_map<DWORD, pair<string, Position>> &playerMap)
{
    // Compute leaders for average and total points
    for (unordered_map<DWORD, PlayerScoringHistory>::iterator itPlayers = franchisePtr->playerStarts.begin(); itPlayers != franchisePtr->playerStarts.end(); itPlayers++)
    {
        DWORD playerID = itPlayers.operator*().first;
        Position posID = playerMap[playerID].second;

        DWORD playerPoints = itPlayers.operator*().second.points;

        if (playerPoints > franchisePtr->highestTotal[posID].second)
            franchisePtr->highestTotal[posID] = make_pair(playerID, playerPoints);

        DWORD playerStarts = itPlayers.operator*().second.starts;

        if (playerStarts > franchisePtr->mostStarts[posID].second)
            franchisePtr->mostStarts[posID] = make_pair(playerID, playerStarts);


        if (playerStarts >= 3)
        {
            float avgPoints = (float)itPlayers.operator*().second.points / playerStarts;

            if (avgPoints > franchisePtr->highestAvg[posID].second)
                franchisePtr->highestAvg[posID] = make_pair(playerID, avgPoints);
        }
    }


    cout << " " << franchisePtr->franchiseName << "  ***  Hall of Fame (" << to_string(YEAR_0) << "-" << to_string(YEAR_N) << ")" << endl;
    cout << " ===========================================================================" << endl;
    cout << setw(24) << "" << setw(26) << left << "Player" << setw(12) << "Points" << setw(12) << "Pts/start" << setw(12) << "Starts" << "Years" << endl;

    for (Position p = Position::QB; p < Position::MAX; p++)
    {
        PlayerScoringHistory* highestTotalPlayer = &(franchisePtr->playerStarts[(franchisePtr->highestTotal)[p].first]);

        string yearsTotal = highestTotalPlayer->printYears();
        float avgPoints = (float)highestTotalPlayer->points / highestTotalPlayer->starts;

        cout << " Best " << Position2str[p] << " (total pts):\t" << setw(26) << left << playerMap[(franchisePtr->highestTotal)[p].first].first
            << setw(12) << (franchisePtr->highestTotal)[p].second << setprecision(3) << setw(12) << avgPoints << setw(12)
            << highestTotalPlayer->starts << yearsTotal << endl;


        PlayerScoringHistory* highestAvgPlayer = &(franchisePtr->playerStarts[(franchisePtr->highestAvg)[p].first]);

        string yearsAvg = highestAvgPlayer->printYears();

        cout << " (avg, min 3 starts):\t" << setw(26) << left << playerMap[(franchisePtr->highestAvg)[p].first].first
            << setw(12) << highestAvgPlayer->points << setprecision(3) << setw(12) << (franchisePtr->highestAvg)[p].second << setw(12)
            << highestAvgPlayer->starts << yearsAvg << endl << endl;

    }

    cout << endl;

    cout << " Average scoring by position" << endl;
    cout << left << setw(20) << " Year" << setw(12) << "QB" << setw(12) << "RB" << setw(12) << "WR" << setw(12) << "TE" << setw(12) << "PK" << "Def" << endl;

    for (int year = YEAR_0; year <= YEAR_N; year++)
    {
        cout << left << setw(20) << " " + to_string(year);

        for (Position p = Position::QB; p < Position::MAX; p++)
        {
            PlayerScoringHistory* posScoringHist = &(franchisePtr->positionalScoring[p][year - YEAR_0]);

            float avgPosition = (float)posScoringHist->points / posScoringHist->starts;

            cout << left << setprecision(3) << setw(12) << avgPosition;

            franchisePtr->positionalScoringTotal[p].points += posScoringHist->points;
            franchisePtr->positionalScoringTotal[p].starts += posScoringHist->starts;
        }

        cout << endl;
    }

    cout << endl << left << setw(20) << " Total";

    for (Position p = Position::QB; p < Position::MAX; p++)
    {
        PlayerScoringHistory* posScoringHist = &(franchisePtr->positionalScoringTotal[p]);

        float avgPosition = (float)posScoringHist->points / posScoringHist->starts;

        cout << left << setprecision(3) << setw(12) << avgPosition;

    }

    cout << endl << endl << endl << endl;


}



void generateRankings(franchiseData** franchiseDirectory, unordered_map<DWORD, pair<string, Position>> &playerMap)
{
    std::multimap<float, int> posRankings[Position::MAX];


    for (Position p = Position::QB; p < Position::MAX; p++)
    {
        for (int teamID = 1; teamID <= 16; teamID++)
        {
            PlayerScoringHistory* posScoringHist = franchiseDirectory[teamID]->positionalScoringTotal;

            float avgPosition = (float)posScoringHist[p].points / posScoringHist[p].starts;

            posRankings[p].insert({ avgPosition, teamID });
        }


        cout << " " << Position2str[p] << " scoring per game" << endl;
        cout << " ===============================================================================================" << endl;
        cout << " Rank\t" << left << setw(30) << "Team" << setw(12) << "Average" << setw(24) << "Most used" << setw(10) << "Starts" << setw(16) << "Players used" << "Non-starts" << endl;

        int rank = 1;

        for (std::multimap<float, int>::reverse_iterator itMultimap = posRankings[p].rbegin(); itMultimap != posRankings[p].rend(); itMultimap++)
        {
            franchiseData* franchisePtr = franchiseDirectory[itMultimap->second];

            DWORD posMultiplier = ((p == RB) || (p == WR)) + 1;
            DWORD nonStarts = (1 + YEAR_N - YEAR_0) * 14 * posMultiplier - franchisePtr->numStarts[p];

            cout << " " << rank << "\t" << left << setw(30) << franchisePtr->franchiseName << setw(12) << setprecision(3) << itMultimap->first
                << setw(24) << playerMap[franchisePtr->mostStarts[p].first].first << setw(10) << franchisePtr->mostStarts[p].second 
                << setw(16) << franchisePtr->playersUsed[p] << nonStarts << endl;

            rank++;
        }

        cout << endl << endl;
    }


    std::multimap<float, int> teamScoringRankings;

    for (int teamID = 1; teamID <= 16; teamID++)
    {
        float avgScore = (float)franchiseDirectory[teamID]->teamScoring.points / franchiseDirectory[teamID]->teamScoring.starts;

        teamScoringRankings.insert({ avgScore, teamID });


        float meanSquareSum = 0;

        for (vector<BYTE>::iterator itScores = franchiseDirectory[teamID]->vectorScores.begin(); itScores != franchiseDirectory[teamID]->vectorScores.end(); itScores++)
        {
            meanSquareSum += pow((itScores.operator*() - avgScore), 2);
        }

        float avgSquaredDifference = meanSquareSum / franchiseDirectory[teamID]->vectorScores.size();
        franchiseDirectory[teamID]->stdDevScores = sqrt(avgSquaredDifference);

    }


    cout << " Team scoring per game" << endl;
    cout << " ===============================================================================================" << endl;
    cout << " Rank\t" << left << setw(30) << "Team" << setw(12) << "Points for" << setw(12) << "Against" << setw(12) << "W-L-T"
        << setw(12) << "Win %" << setw(12) << "StdDev" << endl;

    int rank = 1;

    for (std::multimap<float, int>::reverse_iterator itMultimap = teamScoringRankings.rbegin(); itMultimap != teamScoringRankings.rend(); itMultimap++)
    {
        franchiseData* franchisePtr = franchiseDirectory[itMultimap->second];

        string wltStr(to_string(franchisePtr->teamWins) + "-" + to_string(franchisePtr->teamLosses) + "-" + to_string(franchisePtr->teamTies));
        float winPercentage = (float)franchisePtr->teamWins / (franchisePtr->teamWins + franchisePtr->teamLosses);
        float oppScoringAvg = (float)franchisePtr->oppScoring.points / franchisePtr->oppScoring.starts;

        cout << " " << rank << "\t" << left << setw(30) << franchisePtr->franchiseName << setprecision(3) << setw(12) << itMultimap->first 
            << setw(12) << oppScoringAvg << setw(12) << wltStr << setw(12) << winPercentage << franchisePtr->stdDevScores << endl;

        rank++;
    }

    cout << endl << endl;


    std::multimap<DWORD, DWORD> badCoachingRankings;

    for (int teamID = 1; teamID <= 16; teamID++)
    {
        DWORD size = franchiseDirectory[teamID]->leadingBenchScorers.size();
        badCoachingRankings.insert({ size, teamID });
    }


    cout << " Most times with leading scorer on bench" << endl;
    cout << " ===============================================================================================" << endl;
    cout << " Rank\t" << left << setw(30) << "Team" << setw(12) << "# weeks" << setw(24) << "Highest bench scorer" << setw(12) << "Points"
        << setw(12) << "Week" << endl;

    rank = 1;

    for (std::multimap<DWORD, DWORD>::reverse_iterator itMultimap = badCoachingRankings.rbegin(); itMultimap != badCoachingRankings.rend(); itMultimap++)
    {
        franchiseData* franchisePtr = franchiseDirectory[itMultimap->second];

        multimap<DWORD, tuple<DWORD, DWORD, BYTE>>::reverse_iterator maxBenchScorer = franchisePtr->leadingBenchScorers.rbegin();
        tuple<DWORD, DWORD, BYTE> tupleMaxBenchScorer = maxBenchScorer->second;
        string playerName = playerMap[get<0>(tupleMaxBenchScorer)].first;
        DWORD year = get<1>(tupleMaxBenchScorer);
        DWORD week = get<2>(tupleMaxBenchScorer);

        cout << " " << rank << "\t" << left << setw(30) << franchisePtr->franchiseName << setw(12) << franchisePtr->leadingBenchScorers.size()
            << setw(24) << playerName << setw(12) << maxBenchScorer->first << year << " - week " << week << endl;

        rank++;
    }

    cout << endl << endl;

       
    return;

}





