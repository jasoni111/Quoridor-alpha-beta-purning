#include <vector>
#include <unordered_map>
#include <set>
#include <queue>
#include <iostream>
#include <sstream>
#include <cstring>
#include <cassert>
#include <limits>

// #include <chrono>
// #include <thread>

namespace GAMEAI
{
    const int MYAI = 0; // Representation of my AI
    const int OPPONENT = 1;
    const int INF = 2147483647; // Infinity
    const int ai_side = 0;
    constexpr int BOARDSIZE = 7;
    constexpr int ACTIONSIZE = (BOARDSIZE) * (BOARDSIZE) + (BOARDSIZE - 1) * (BOARDSIZE - 1) * 2;
    constexpr int STATESIZE = (BOARDSIZE * 2 - 1) * (BOARDSIZE * 2 - 1) + 8 * 2;

    const int deltax[12] = {0, 0, -1, 1, -1, 1, -1, 1, 0, 0, -2, 2};
    const int deltay[12] = {-1, 1, 0, 0, -1, 1, 1, -1, -2, 2, 0, 0}; // Delta of pawn moving
    const int LEFT = 0;
    const int RIGHT = 1;
    const int UP = 2;
    const int DOWN = 3;
    const int UPLEFT = 4;
    const int DOWNRIGHT = 5;
    const int UPRIGHT = 6;
    const int DOWNLEFT = 7;
    const int LEFTLEFT = 8;
    const int RIGHTRIGHT = 9;
    const int UPUP = 10;
    const int DOWNDOWN = 11;
    //--------------------------------Variables Defining--------------------------------
    // type Action
    //<int, <int, int>> : <evaluation, <moveposx, moveposy>>
    // moveposx, moveposy:
    //	(even, even) --> pawn moving
    // 	(even, odd)  --> vertitcal wall placing
    //	(odd, even)  --> horrizontal wall placing
    //  (odd, odd)   --> illegal action
    using std::cerr;
    using std::endl;
    using std::make_pair;
    using std::pair;
    typedef pair<double, pair<int, int>> Move;
    Move bestaction;
    int NOWDEPTH;
    bool debugflag = true;

    // type QuoridorBoard
    class QuoridorBoard
    {
    public:
        class Chess
        {
        public:
            enum ChessType
            {
                WhiteChess,
                BlackChess,
                VerticalWall,
                HorizontalWall
            };
            int x; // 0..7 for chess, 0..6 for wall
            int y; // 0..7 for chess, 0..6 for wall
            ChessType type;
        };

        Chess get_target_chess_from_move(const Move &move) const
        {
            Chess to_return;
            if (IsMovingThePawn(move.second))
            {
                to_return.type = Chess::ChessType::WhiteChess;
                to_return.x = (move.second.first / 2) - 1;
                to_return.y = (move.second.second / 2) - 1;
            }
            else if (IsVertical(move.second))
            {
                to_return.type = Chess::ChessType::VerticalWall;
                to_return.x = (move.second.first) / 2 - 1;
                to_return.y = (move.second.second - 1) / 2 - 1;
            }
            else
            {
                to_return.type = Chess::ChessType::HorizontalWall;
                to_return.x = (move.second.first - 1) / 2 - 1;
                to_return.y = (move.second.second) / 2 - 1;
            }

            return to_return;
        }

        void move_enemy_chess(Chess chess)
        {
            if (chess.type == QuoridorBoard::Chess::ChessType::BlackChess)
            {
                location[1] = make_pair<int, int>(chess.x * 2 + 2, chess.y * 2 + 2);
            }
            if (chess.type == QuoridorBoard::Chess::ChessType::VerticalWall)
            {
                int x = chess.x * 2 + 2;
                int y = chess.y * 2 + 3;
                walls[x][y] = walls[x + 1][y] = walls[x + 2][y] = 1;
                --wallcnt[1];
            }
            if (chess.type == QuoridorBoard::Chess::ChessType::HorizontalWall)
            {
                int x = chess.x * 2 + 3;
                int y = chess.y * 2 + 2;
                walls[x][y] = walls[x][y + 1] = walls[x][y + 2] = 1;
                --wallcnt[1];
            }
        }

        pair<int, int> location[2];

    private:
        static inline int action_key(pair<int, int> key)
        {
            return (int)key.first << 16 | (unsigned int)key.second;
        }
        static inline int action_key(int first, int second)
        {
            return (int)first << 16 | (unsigned int)second;
        }

        static std::unordered_map<int, int> action_map;
        static std::unordered_map<int, pair<int, int>> reverse_action_map;

        static bool HasActionMapInited;

        static void InitActionMap()
        {
            if (HasActionMapInited)
                return;
            reverse_action_map.reserve(BOARDSIZE * BOARDSIZE + (BOARDSIZE - 1) * (BOARDSIZE - 1) * 2);
            action_map.reserve(BOARDSIZE * BOARDSIZE + (BOARDSIZE - 1) * (BOARDSIZE - 1) * 2);
            int count = 0;
            for (int i = 2; i <= BOARDSIZE * 2; i += 2)
            {
                for (int j = 2; j <= BOARDSIZE * 2; j += 2)
                {
                    reverse_action_map.insert(std::make_pair(count, make_pair(i, j)));
                    action_map.insert(std::make_pair(action_key(i, j), count++));
                }
            }
            for (int row = 2; row <= BOARDSIZE * 2 - 1; row++)
                for (int col = 3 - (row & 1); col <= BOARDSIZE * 2 - 1; col += 2)
                {
                    reverse_action_map.insert(std::make_pair(count, make_pair(row, col)));
                    action_map.insert(std::make_pair(action_key(row, col), count++));
                }
            HasActionMapInited = true;
        }

        int wallcnt[2], destrow[2];
        short int walls[BOARDSIZE * 2 + 2][BOARDSIZE * 2 + 2];
        static int vis[BOARDSIZE * 2 + 2][BOARDSIZE * 2 + 2];
        static pair<int, int> que[100];
        int head, tail;

        inline Move ConstructMove(const int &evaluation, const pair<int, int> &move) const
        {
            return make_pair(evaluation, move);
        }

        static inline bool IsMovingThePawn(const pair<int, int> &move)
        {
            return (!(move.first & 1)) && (!(move.second & 1));
        }
        static inline bool IsVertical(const pair<int, int> &move)
        {
            return (!(move.first & 1)) && (move.second & 1);
        }
        static inline bool IsInsideTheBoard(const pair<int, int> &move)
        {
            int x = move.first, y = move.second;
            return (2 <= x) && (x <= BOARDSIZE * 2) && (2 <= y) && (y <= BOARDSIZE * 2);
        }
        inline pair<int, int> CalDeltaPawnPosition(const pair<int, int> &now, const int &direction) const
        {
            return make_pair(now.first + deltax[direction], now.second + deltay[direction]);
        }
        inline bool const IsOccupiedByWall(const pair<int, int> &now, const int &direction) const
        {
            return walls[now.first + deltax[direction]][now.second + deltay[direction]];
        }
        inline bool IsLegalPawnMoving(const pair<int, int> &now, const pair<int, int> &destination, const int &direction, const int &player) const
        {
            if (location[player ^ 1] == destination)
                return false;
            if (!IsInsideTheBoard(destination))
                return false;
            // up down left right
            if (direction < 4)
                return !IsOccupiedByWall(now, direction);
            else if (direction < 8)
            {
                // upright
                if (direction == UPRIGHT)
                {
                    // opponent in up direction
                    if (location[player ^ 1] == CalDeltaPawnPosition(now, UPUP))
                    {
                        if (!IsOccupiedByWall(location[player ^ 1], UP))
                            return false;
                        if (IsOccupiedByWall(location[player ^ 1], DOWN))
                            return false;
                        if (IsOccupiedByWall(location[player ^ 1], RIGHT))
                            return false;
                        return true;
                    }
                    // opponent in right direction
                    if (location[player ^ 1] == CalDeltaPawnPosition(now, RIGHTRIGHT))
                    {

                        if (!IsOccupiedByWall(location[player ^ 1], RIGHT))
                            return false;

                        if (IsOccupiedByWall(location[player ^ 1], LEFT))
                            return false;
                        if (IsOccupiedByWall(location[player ^ 1], UP))
                            return false;
                        return true;
                    }
                    return false;
                }
                // upleft
                if (direction == UPLEFT)
                {
                    // opponent in up direction
                    if (location[player ^ 1] == CalDeltaPawnPosition(now, UPUP))
                    {

                        if (!IsOccupiedByWall(location[player ^ 1], UP))
                            return false;

                        if (IsOccupiedByWall(location[player ^ 1], DOWN))
                            return false;
                        if (IsOccupiedByWall(location[player ^ 1], LEFT))
                            return false;
                        return true;
                    }
                    // opponent in left direction
                    if (location[player ^ 1] == CalDeltaPawnPosition(now, LEFTLEFT))
                    {
                        if (!IsOccupiedByWall(location[player ^ 1], LEFT))
                            return false;

                        if (IsOccupiedByWall(location[player ^ 1], UP))
                            return false;
                        if (IsOccupiedByWall(location[player ^ 1], RIGHT))
                            return false;
                        return true;
                    }
                    return false;
                }
                // downright
                if (direction == DOWNRIGHT)
                {
                    // opponent in down direction
                    if (location[player ^ 1] == CalDeltaPawnPosition(now, DOWNDOWN))
                    {
                        if (!IsOccupiedByWall(location[player ^ 1], DOWN))
                            return false;
                        if (IsOccupiedByWall(location[player ^ 1], UP))
                            return false;
                        if (IsOccupiedByWall(location[player ^ 1], RIGHT))
                            return false;
                        return true;
                    }
                    // opponent in right direction
                    if (location[player ^ 1] == CalDeltaPawnPosition(now, RIGHTRIGHT))
                    {
                        if (!IsOccupiedByWall(location[player ^ 1], RIGHT))
                            return false;
                        if (IsOccupiedByWall(location[player ^ 1], DOWN))
                            return false;
                        if (IsOccupiedByWall(location[player ^ 1], LEFT))
                            return false;
                        return true;
                    }
                    return false;
                }
                // downleft
                if (direction == DOWNLEFT)
                {
                    // opponent in down direction
                    if (location[player ^ 1] == CalDeltaPawnPosition(now, DOWNDOWN))
                    {
                        if (!IsOccupiedByWall(location[player ^ 1], DOWN))
                            return false;
                        if (IsOccupiedByWall(location[player ^ 1], LEFT))
                            return false;
                        if (IsOccupiedByWall(location[player ^ 1], UP))
                            return false;
                        return true;
                    }
                    // opponent in left direction
                    if (location[player ^ 1] == CalDeltaPawnPosition(now, LEFTLEFT))
                    {
                        if (!IsOccupiedByWall(location[player ^ 1], LEFT))
                            return false;
                        if (IsOccupiedByWall(location[player ^ 1], DOWN))
                            return false;
                        if (IsOccupiedByWall(location[player ^ 1], RIGHT))
                            return false;
                        return true;
                    }
                    return false;
                }
            }
            else
            {
                // upup, downdown, leftleft, rightright
                return (location[player ^ 1] == CalDeltaPawnPosition(now, direction)) && (!IsOccupiedByWall(now, direction - 8)) && (!IsOccupiedByWall(location[player ^ 1], direction - 8));
            }
            return false;
        }
        inline int BreadthFirstSearch(const int &player)
        {
            pair<int, int> tmplocation = location[player ^ 1];
            location[player ^ 1] = make_pair(1, 1);
            memset(vis, -1, sizeof(vis));
            que[head = tail = 1] = location[player];
            vis[location[player].first][location[player].second] = 0;
            for (; head <= tail; head++)
            {
                pair<int, int> now = que[head];
                for (int index = 0; index < 12; index++)
                {
                    int newx = now.first + (deltax[index] << 1), newy = now.second + (deltay[index] << 1);
                    if (vis[newx][newy] == -1 && IsLegalPawnMoving(now, make_pair(newx, newy), index, player))
                    {
                        que[++tail] = make_pair(newx, newy);
                        vis[newx][newy] = vis[now.first][now.second] + 1;
                    }
                }
            }
            location[player ^ 1] = tmplocation;
            int minlength = INF;
            for (int col = 1; col <= BOARDSIZE * 2; col++)
                if (vis[destrow[player]][col] != -1)
                    minlength = std::min(minlength, vis[destrow[player]][col]);
            return minlength;
        }

        inline bool IsLegalWallPlacing(const pair<int, int> &move, const int &player)
        {
            int x = move.first, y = move.second;
            if (IsVertical(move))
            {
                if (walls[x][y] || walls[x + 1][y] || walls[x + 2][y])
                    return false;
                walls[x][y] = walls[x + 1][y] = walls[x + 2][y] = 1;
                int minlength = std::max(BreadthFirstSearch(player), BreadthFirstSearch(player ^ 1));
                walls[x][y] = walls[x + 1][y] = walls[x + 2][y] = 0;
                return minlength != INF;
            }
            else
            {
                if (walls[x][y] || walls[x][y + 1] || walls[x][y + 2])
                    return false;
                walls[x][y] = walls[x][y + 1] = walls[x][y + 2] = 1;
                int minlength = std::max(BreadthFirstSearch(player), BreadthFirstSearch(player ^ 1));
                walls[x][y] = walls[x][y + 1] = walls[x][y + 2] = 0;
                return minlength != INF;
            }
        }
        inline void GeneratePawnMoving(std::vector<Move> &listofactions, const int &player) const
        {
            for (int index = 0; index < 12; index++)
            {
                int x = location[player].first + (deltax[index] << 1);
                int y = location[player].second + (deltay[index] << 1);
                if (IsLegalPawnMoving(location[player], make_pair(x, y), index, player))
                    listofactions.push_back(ConstructMove(-1, make_pair(x, y)));
            }
        }
        inline void GenerateWallPlacing(std::vector<Move> &listofactions, const int &player) const
        {
            QuoridorBoard temp(*this);
            for (int row = 2; row <= BOARDSIZE * 2 - 1; row++)
                for (int col = 3 - (row & 1); col <= BOARDSIZE * 2 - 1; col += 2)
                {
                    if (temp.IsLegalWallPlacing(make_pair(row, col), player))
                        listofactions.push_back(ConstructMove(-1, make_pair(row, col)));
                }
        }
        struct AStarNode
        {
            int ManhattanDistance;
            float distance;
            std::pair<int, int> first_move{-1, -1};
            std::pair<int, int> last_move;
            // std::vector<std::pair<int, int>> moves;
            bool operator<(const AStarNode &another) const
            {
                return distance > another.distance;
            }
        };

    public:
        int NotAStar(const int &player) const
        {
            if (player != 0)
            {
                assert(true);
                assert(false);
            }
            int min_distance = 19999;
            std::pair<int, int> best_action;
            for (int index = 0; index < 12; index++)
            {
                int newx = location[player].first + (deltax[index] << 1);
                int newy = location[player].second + (deltay[index] << 1);
                if (IsLegalPawnMoving(location[player], make_pair(newx, newy), index, player))
                {
                    QuoridorBoard copy(*this);
                    copy.MakeAction(std::pair<int, int>(newx, newy), player);
                    auto distance = copy.BreadthFirstSearch(player);
                    if (min_distance > distance)
                    {
                        min_distance = distance;
                        best_action = std::pair<int, int>(newx, newy);
                    }
                }
            }
            return action_map.find(action_key(best_action))->second;
        }
        // int depthlimitAlphaBeta() {}

        double evulation() const
        {
            QuoridorBoard copy(*this);
            double to_return = (double)((double)copy.BreadthFirstSearch(1) * 0.7999f - copy.BreadthFirstSearch(0)) / 100.0f;
            assert(to_return >= -1 && to_return <= 1);
            return to_return;
        }

        bool inline HasBoardLeft(const int &player = 0) const
        {
            return wallcnt[player];
        }

        bool inline IsBoothEndGame() const
        {
            if (location[0].first == destrow[0] && location[1].first == destrow[1])
            {
                return true;
            }
            return 0;
        }

        double inline IsEndGame() const
        {

            //    QuoridorBoard copy(*this);
            // double to_return = (double)((double)copy.BreadthFirstSearch(1) * 0.7999f - copy.BreadthFirstSearch(0)) / 100.0f;
            // assert(to_return >= -1 && to_return <= 1);
            // return to_return;
            if (location[0].first == destrow[0])
            {
                QuoridorBoard copy(*this);
                double to_return = (double)copy.BreadthFirstSearch(1);
                // assert(to_return >= -1 && to_return <= 1);
                return to_return;
            }
            // return 1;
            if (location[1].first == destrow[1])
            {
                QuoridorBoard copy(*this);
                double to_return = (double)-copy.BreadthFirstSearch(0);
                // assert(to_return >= -1 && to_return <= 1);
                return to_return;
            }
            // return -1;
            return 0;
        }
        // Initialize the Board
        inline void Initialize()
        {
            if (!HasActionMapInited)
                InitActionMap();
            // Set the coordinate of the pawn and its destination
            location[ai_side] = make_pair(2, BOARDSIZE + 1);
            location[ai_side ^ 1] = make_pair(BOARDSIZE * 2, BOARDSIZE + 1);
            destrow[ai_side] = BOARDSIZE * 2;
            destrow[ai_side ^ 1] = 2;
            // Set the wallcounts
            wallcnt[0] = wallcnt[1] = 8;
            // Set the wallplacements
            memset(walls, 0, sizeof(walls));
            for (int i = 1; i <= BOARDSIZE * 2 + 1; i++)
                walls[1][i] = walls[BOARDSIZE * 2 + 1][i] = walls[i][1] = walls[i][BOARDSIZE * 2 + 1] = 1;
        }

        // Show the board state for debug
        inline void Show() const
        {
            cerr << "myloc : " << location[MYAI].first << " " << location[MYAI].second << endl;
            cerr << "oploc : " << location[OPPONENT].first << " " << location[OPPONENT].second << endl;
            cerr << "mydestrow : " << destrow[MYAI] << endl;
            cerr << "opdestrow : " << destrow[OPPONENT] << endl;
            cerr << "mywallcnt : " << wallcnt[MYAI] << endl;
            cerr << "opwallcnt : " << wallcnt[OPPONENT] << endl;
            for (int i = 1; i <= BOARDSIZE * 2 + 1; ++i)
            {
                for (int j = 1; j <= BOARDSIZE * 2 + 1; ++j)
                {
                    if (i == location[MYAI].first && j == location[MYAI].second)
                        cerr << "W"
                             << " ";
                    else if (i == location[OPPONENT].first && j == location[OPPONENT].second)
                        cerr << "B"
                             << " ";
                    else if (!walls[i][j])
                    {
                        cerr << ". ";
                    }
                    else
                        cerr << walls[i][j] << " ";
                }
                cerr << endl;
            }
        }
        // Generate all possible actions for the board
        inline void GenerateAction(std::vector<Move> &listofactions, const int &player) const
        {
            // Moving the pawn
            GeneratePawnMoving(listofactions, player);
            // Placing walls
            if (wallcnt[player] != 0)
                GenerateWallPlacing(listofactions, player);
        }
        // Apply the action to the board
        inline void MakeAction(const pair<int, int> &move, const int &player)
        {
            if (player)
            {
                assert(true);
                assert(false);
            }
            if (IsMovingThePawn(move))
                location[player] = move;
            else
            {
                wallcnt[player]--;
                int x = move.first, y = move.second;
                if (IsVertical(move))
                    walls[x][y] = walls[x + 1][y] = walls[x + 2][y] = 1;
                else
                    walls[x][y] = walls[x][y + 1] = walls[x][y + 2] = 1;
            }
        }

        // //Apply the action to the board
        // inline void MakeAction(const int &move_int, const int &player)
        // {
        //     if (!player)
        //     {
        //         MakeAction((*reverse_action_map.find(move_int)).second, player);
        //     }
        //     else
        //     {

        //     }
        // }

        std::string GetCompareString() const
        {
            std::ostringstream os;
            for (int i = 0; i < BOARDSIZE * 2 + 2; ++i)
            {
                for (int j = 0; j < BOARDSIZE * 2 + 2; ++j)
                {
                    os << walls[i][j];
                }
            }
            os << location[0].first << location[0].second;
            os << location[1].first << location[1].second;

            os << wallcnt[0] << wallcnt[1];

            return std::string(os.str());
        }

        QuoridorBoard() = default;
        QuoridorBoard(const QuoridorBoard &another) = default;

        QuoridorBoard GetCanonicalBoard(int player) const
        {
            if (!player)
                return *this;
            else
                return QuoridorBoard(*this, true);
        }

        static int getIntFromMove(const Move &move)
        {
            return action_map.find(action_key(move.second))->second;
        }

    private:
        QuoridorBoard(const QuoridorBoard &another, bool filp) : location{another.location[1], another.location[0]},
                                                                 wallcnt{another.wallcnt[1], another.wallcnt[0]},
                                                                 destrow{another.destrow[0], another.destrow[1]},
                                                                 walls()
        {
            location[0].first = BOARDSIZE * 2 + 2 - location[0].first;
            location[1].first = BOARDSIZE * 2 + 2 - location[1].first;

            for (int i = 0; i < BOARDSIZE * 2 + 2; ++i)
            {
                for (int j = 0; j < BOARDSIZE * 2 + 2; ++j)
                {
                    walls[i][j] = another.walls[BOARDSIZE * 2 + 2 - i][j];
                }
            }
        }
    };

    // define static variables
    int QuoridorBoard::vis[BOARDSIZE * 2 + 2][BOARDSIZE * 2 + 2];
    pair<int, int> QuoridorBoard::que[100];
    std::unordered_map<int, pair<int, int>> QuoridorBoard::reverse_action_map;

    std::unordered_map<int, int> QuoridorBoard::action_map;
    bool QuoridorBoard::HasActionMapInited = false;

    Move DepthLimitNegamax(const QuoridorBoard &canonicalBoard, int depth, int player,
                           double alpha = -1.0f, double beta = 1.0f)
    {
        if (player != 0)
        {
            return DepthLimitNegamax(canonicalBoard.GetCanonicalBoard(1), depth, 0);
        }
        if (canonicalBoard.IsEndGame())
        {
            Move move;
            move.first = -canonicalBoard.IsEndGame();
            return move;
            // return std::pair<int, int>(canonicalBoard.IsEndGame(), -1);
        }
        if (depth == 0)
        {
            Move move;
            move.first = -canonicalBoard.evulation();
            return move;
        }

        std::vector<Move> ValidMoves;
        canonicalBoard.GenerateAction(ValidMoves, 0);
        Move BestMove(-std::numeric_limits<double>::infinity(), std::pair<int, int>(-1, -1));

        for (Move move : ValidMoves)
        {
            QuoridorBoard nextBoard(canonicalBoard);
            nextBoard.MakeAction(move.second, 0);
            Move nextMove = DepthLimitNegamax(nextBoard.GetCanonicalBoard(1), depth - 1, 0, -beta, -alpha);

            if (nextMove.first > BestMove.first)
            {
                BestMove.first = nextMove.first;
                BestMove.second = move.second;
            }
            alpha = std::max(alpha, BestMove.first);
            if (alpha >= beta)
            {
                break;
            }
        }
        BestMove.first *= -0.99f;
        return BestMove;
    }

    void Alphabetapruning()
    {
    }

} // namespace GAMEAI

int main()
{
    using namespace GAMEAI;
    QuoridorBoard Q;
    Q.Initialize();
    bool fp = true;
    QuoridorBoard::Chess chess;
    chess.type = QuoridorBoard::Chess::BlackChess;
    chess.x = 0;
    chess.y = 0;

    Q.Show();

    QuoridorBoard::Chess NextChess;
    Move NextMove = DepthLimitNegamax(Q, 3, 0);
    NextChess = Q.get_target_chess_from_move(NextMove);
    Q.MakeAction(NextMove.second, 0);

    Q.Show();
    // while (!Q.IsEndGame())
    // {
    //     // std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    //     if (fp)
    //     {
    //         Q.MakeAction(DepthLimitNegamax(Q, 3, 0).second, 0);
    //         Q.Show();
    //     }
    //     else
    //     {
    //         QuoridorBoard temp = Q.GetCanonicalBoard(1);
    //         temp.MakeAction(DepthLimitNegamax(temp, 3, 0).second, 0);
    //         Q = temp.GetCanonicalBoard(1);
    //         Q.Show();
    //     }
    //     fp = !fp;
    // }

    // DepthLimitNegamax(Q,1);
    // std::cout<<DepthLimitNegamax(Q,2).first<<std::endl;
    // Q.MakeAction(DepthLimitNegamax(Q,2).second,0);
    return 0;
}