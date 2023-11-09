#include <stdio.h>
#include <string>
#include <vector>
#include <algorithm>

#include "profiler.h"

#include "ZSkipList.h"

struct Player
{
	std::string m_Name;
	double m_Score;
	long long m_Rank{};
};

struct F
{
	long long operator()(Player* a, Player* b)
	{
		return (long long)(a)-(long long)(b);
	}
};

int main()
{
	const int MAX_USER = 10;
	const bool PRINT = true;
	std::vector<Player*> PlayerList;
	ZSkipList<Player*, F> zz;

	for (int i = 0; i < MAX_USER; ++i)
	{
		Player* p = new Player;
		p->m_Name = std::to_string(i);
		p->m_Score = rand();

		zz.Insert(p->m_Score, p);
		PlayerList.push_back(p);
	}

	printf("* forward\n");
	{
		long long Rank = 1;
		auto Node = zz.Head();
		while (Node != nullptr)
		{
			Player* P = Node->ele;
			if(PRINT)
				printf("name:%s, rank:%I64d, score:%lf\n", P->m_Name.c_str(), Rank++, Node->score);
			Node = Node->Forward();
		}
	}

	printf("* backward\n");
	{
		long long Rank = 1;
		auto Node = zz.Tail();
		while (Node != nullptr)
		{
			Player* P = Node->ele;
			if (PRINT)
				printf("name:%s, rank:%I64d, score:%lf\n", P->m_Name.c_str(), Rank++, Node->score);
			Node = Node->Backward();
		}
	}

	printf("* rank\n");
	{
		long long Rank = 1;
		for (auto P : PlayerList)
		{
			if (PRINT)
				printf("name:%s, rank:%I64d, score:%lf\n", P->m_Name.c_str(), zz.GetRank(P->m_Score, P), P->m_Score);
		}
	}

	printf("* update\n");
	{
		cProfiler c("update");
		long long Rank = 1;
		for(auto P : PlayerList)
		{
			double Old = P->m_Score;
			int Score = rand();				
			zz.UpdateScore(P->m_Score, P, Score);

			P->m_Score = Score;
			if (PRINT)
				printf("name:%s, rank:%I64d - score:%lf, old_score:%lf\n", P->m_Name.c_str(), zz.GetRank(P->m_Score, P), P->m_Score, Old);
		}
	}

	{
		printf("* update(sort)\n");
		cProfiler c("update");

		for (int i = 0; i < MAX_USER; ++i)
		{
			int score = rand();
			PlayerList[i]->m_Score = score;

			Player* P = PlayerList[i];

			std::sort(PlayerList.begin(), PlayerList.end(), [](const auto& l, const auto& r)
				{ return l->m_Score < r->m_Score; });
		}
	}


	printf("* rank(sort)\n");
	{
		for (int i = 0; i < MAX_USER; ++i)
		{
			PlayerList[i]->m_Rank = i + 1;
			Player* P = PlayerList[i];
			if (PRINT)
				printf("name:%s, rank:%I64d, score:%lf\n", P->m_Name.c_str(), P->m_Rank, P->m_Score);
		}
	}

}
