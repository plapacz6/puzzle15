/*
author: plapacz6@gmail.com
data: 2020-07-16
ver: 0.1.0
*/
  #include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <iostream>
#include <iomanip>
#include <ctime>
#include <cstdlib>

using namespace std;
using namespace cv;
//#define DEBUG_T15
#define T15_USE_HITORY_REJESTR

//namespace my_app {
  const int tile_width_n = 4;
  const int tile_height_n = 4;
  //const int tile_size = tile_width_n * tile_height_n;

  struct {
    Mat t;
    Rect r;
    bool empty;
    int nr;     //original number (for checking if solved)
  } tiles15[tile_height_n][tile_width_n];

  Mat img;
  Mat background;
  Mat current_state;  
  Mat tmp;
  Point move_start, move_end, move_over;
  int tile_width;
  int tile_height;
  int empty_row;
  int empty_col;  

  #ifdef T15_USE_HITORY_REJESTR
  struct move_rejestr_entry_t {    
    int rs;
    int cs;
    int re;
    int ce;
    move_rejestr_entry_t(){      
    }
    move_rejestr_entry_t(int rs_, int cs_, int re_, int ce_): 
      rs(rs_), cs(cs_), re(re_), ce(ce_) {      
      }
    move_rejestr_entry_t(move_rejestr_entry_t&& o){      
      rs = o.rs;
      cs = o.cs;
      re = o.re;
      ce = o.ce;
    }
    move_rejestr_entry_t(move_rejestr_entry_t& o){      
      rs = move(o.rs);
      cs = move(o.cs);
      re = move(o.re);
      ce = move(o.ce);
    }    
    ~move_rejestr_entry_t(){      
    }
  };
  vector<move_rejestr_entry_t> move_backward_rejestr;
  vector<move_rejestr_entry_t> move_user_history;
  #endif // T15_USE_HITORY_REJESTR  

  int moves_counter;
  int border_color_start;
  int border_color_end;
  int border_color_hide;  
  
  int hardness;     //hardness degree
  int background_off = 0;
  int language = 0;  //0 - eng, 1 - pl  
  string info_1[] = {
    "[space] -> again, [esc] -> end  | move: ",
    "[spacja] -> od nowa [esc] -> koniec |Ruch:"
  };
  string info_2[] = {
  #ifdef T15_USE_HITORY_REJESTR
    "PUZZLE SOLVED  ([space] - again, [h] - show history, [esc] - end)",
    "UKLADANKA UŁOZONA ([spacja] -> jesze raz, [h] - historia, [esc] -> zakoncz)"  
  #else
    "PUZZLE SOLVED  ([space] - again, [esc] - end)",
    "UKLADANKA UŁOZONA ([spacja] -> jesze raz, [esc] -> zakoncz)"    
  #endif // T15_USE_HITORY_REJESTR
  };  
  string info_3[] = {
    "can't make a move over several fields",
    "nie mozna wykonac ruchu przez kilka pól"
  };  
  string info_4[] = {
    "can't make a move!", 
    "nie moge wykonac ruchu"
  };
  string info_5[] = {
    "... point into window and press any key to end puzzle15",
    "... kliknij w okno ukladanki i nacisnij [esc] zeby zakonczyc"
  };
  string info_6[] = {
    "can't open puzzle image", 
    "nie moge otworzyc obrazka"
  };
  
  #ifdef T15_USE_HITORY_REJESTR
  string info_8[] = {
    "history of user moves: ",
    "historia ruchów użytkownika: "
  };
  string info_9[] = {
    "reverse history of move during preparing mess:",
    "odwotna historia ruchów podczas mieszania fragmentów układanki:"
  };
  //bool history_printed = false;
  #endif //T15_USE_HITORY_REJESTR  
  
  string gameWindowName = "puzzle15";
  bool is_puzzle_solved;
  
  #ifdef DEBUG_T15
  string debugWindow = "debugWindow";
  #endif // DEBUG_T15
//}


void tile_border(int r, int c, int color){
  //color == 0 - hide  
  if(color == border_color_start) {
    //cout << "click DOWN tile : [row,col] : [" << r << "," << c << "]" << endl;      
  }
  if(color == border_color_end) {
    //cout << "click UP tile : [row,col] : [" << r << "," << c << "]" << endl;      
  }
  if(color == border_color_hide){
    //cout << "click OVER tile : [row,col] : [" << r << "," << c << "]" << endl;     
  }  
}


void pasteMat2Mat(const Mat &src, const Rect &roi, Mat& dst){
  
  #ifdef DEBUG_T15
  // cout << "pasteMat2Mat()"  << endl;  
  // cout << "src.width : " << src.size().width << endl;
  // cout << "src.height : " << src.size().height << endl;
  // cout << "dst.width : " << dst.size().width << endl;
  // cout << "dst.height : " << dst.size().height << endl;
  // cout << "src.channels :" << src.channels() << endl;
  // cout << "dst.channels :" << dst.channels() << endl;
  #endif //DEBUG_T15      
  
  for(int rowS = 0, rowD = roi.y; rowD < roi.y + roi.height; ++rowS, ++rowD){
    for(int colS = 0, colD = roi.x; colD < roi.x + roi.width; ++colS, ++colD){
      //cout << "row, col: " << rowD << "," << colD << endl;
      dst.at<Vec3b>(rowD,colD) = src.at<Vec3b>(rowS,colS);            
    }
  }
}


void create_current_state(){
  #ifdef DEBUG_T15
  cout << "create_current_state()" << endl;
  #endif // DEBUG_T15
  pasteMat2Mat(
    background, 
    Rect(0,0,img.rows, img.cols), 
    current_state);  
  for(int row = 0; row < tile_height_n; row++){
    for(int col = 0; col < tile_width_n; col++){
      //cout << tiles15[row][col].r << endl;
      //cout << tiles15[row][col].t << endl;
      if(!tiles15[row][col].empty){
        pasteMat2Mat(
          tiles15[row][col].t, 
          tiles15[row][col].r,        
          current_state);  
      }
      else{
        //
      }        
    }
  }
}


bool check_if_puzzle_solved(){

  for(int w = 0; w < tile_height_n; w++)
    for(int c = 0; c < tile_width_n; c++){
      if(tiles15[w][c].nr != w * tile_height_n + c){
        return false;
      }
    }
  is_puzzle_solved = true;
  return true;
}


#ifdef T15_USE_HITORY_REJESTR
void print_moves_history(){
  cout << endl << info_8[language] << endl;
  for(int i = 0; i < move_user_history.size(); i++){
    cout  << "  [" <<  setw(2) << i << "]:" << 
    "[" << move_user_history[i].rs + 1 << "," << move_user_history[i].cs + 1 << "] -> " << 
    "[" << move_user_history[i].re + 1 << "," << move_user_history[i].ce + 1 << "]" << endl;
  }
  cout << endl << info_9[language] << endl;
  int last_move = move_backward_rejestr.size() - 1;
  for(int i = last_move; i >= 0; i--){
    cout << "  [" <<  setw(2) << (last_move - i) << "]:" << 
    "[" << move_backward_rejestr[i].re + 1 << "," << move_backward_rejestr[i].ce + 1 << "] -> " << 
    "[" << move_backward_rejestr[i].rs + 1 << "," << move_backward_rejestr[i].cs + 1 << "]" << endl;
    
  }
  //history_printed = true;
}
#endif// T15_USE_HITORY_REJESTR


/**
 * @brief swap content of 2 tiles
 * 
 * @param r1 row of moved tile (not empty)
 * @param c1 
 * @param r2 row of destination tile (must be empty)
 * @param c2 
 */
void swap_tile(int r1, int c1, int r2, int c2){
  CV_Assert(tiles15[r2][c2].empty == true);
  CV_Assert(tiles15[r1][c1].empty == false);
  #ifdef DEBUG_T15
  cout << "swaping tiles[" << r1 << "," << c1 << "] <-> tiles[" << r2 << "," << c2 << "]" << endl;
  #endif // DEBUG_T15
  tiles15[r1][c1].empty = true;
  tiles15[r2][c2].empty = false;
  empty_row = r1;
  empty_col = c1;

  tmp = tiles15[r1][c1].t;
  tiles15[r1][c1].t = tiles15[r2][c2].t;
  tiles15[r2][c2].t = tmp;
  int tmp_nr;
  tmp_nr = tiles15[r1][c1].nr;
  tiles15[r1][c1].nr = tiles15[r2][c2].nr;
  tiles15[r2][c2].nr = tmp_nr;
}


void moveTile(int event, int x, int y, int flags, void* userdata){
  if(is_puzzle_solved) 
    return;
  if(event == EVENT_LBUTTONDOWN){
    move_start = Point(x,y);    
    tile_border(y, x, border_color_start);
  } 
  else if(event == EVENT_MOUSEMOVE){
    tile_border(move_over.y, move_over.x, border_color_hide);
    move_over = Point(x,y);
    tile_border(y, x, border_color_end);
  }  
  else if(event == EVENT_LBUTTONUP){
    move_end = Point(x,y);
    tile_border(move_over.y, move_over.x, border_color_hide);
    tile_border(move_start.y, move_start.x, border_color_hide);
    tile_border(move_end.y, move_end.x, border_color_hide);

    //make a move
    //locate the end tile
    int col_end = move_end.x / tile_width;
    int row_end = move_end.y / tile_height;
    #ifdef DEBUG_T15
    cout << "click B - UP tile : [row,col] : [" << row_end << "," << col_end << "]" << endl;
    #endif //DEBUG_T15

    //check if end tile is empty
    if(tiles15[row_end][col_end].empty){    
      //locate the start tile
      int col_start = move_start.x / tile_width;
      int row_start = move_start.y / tile_height;
      #ifdef DEBUG_T15
      cout << "click A - DOWN tile : [row,col] : [" << row_start << "," << col_start << "]" << endl;    
      cout << "abs r1 - r2 :"  << std::abs(row_end - row_start) << endl;
      cout << "abs c1 - c2 :"  << std::abs(col_end - col_start) << endl;
      #endif // DEBUG_T15
      if(
        (std::abs(row_end - row_start) == 1 && col_end == col_start )
        ||
        (row_end == row_start && std::abs(col_end - col_start) == 1)
      ){
        tile_border(row_start, col_start, border_color_start);
        moves_counter++;      
        swap_tile(row_start, col_start, row_end, col_end);
        #ifdef T15_USE_HITORY_REJESTR
        move_user_history.emplace_back(move_rejestr_entry_t(row_start, col_start, row_end, col_end));
        #endif // T15_USE_HITORY_REJESTR
        setWindowTitle(gameWindowName, info_1[language] + to_string(moves_counter));
        create_current_state();        
        imshow(gameWindowName, current_state);
      }
      else {
        cout << info_3[language] << endl;
        
      }                  
    }
    else {
      cout << info_4[language] << endl;
    }            
  }
  check_if_puzzle_solved();
  if(is_puzzle_solved){    
    cout << endl << info_2[language] << endl;
    
    setWindowTitle(gameWindowName, info_2[language]);
  }
}


/**
 * @brief prepare mess on board
 * 
 */
void make_mess_on_board(){
  //15 moves making mess on board
  #ifdef T15_USE_HITORY_REJESTR
  move_backward_rejestr.clear();
  #endif // T15_USE_HITORY_REJESTR  
  int prev_direction = 5;
  for(int i = 0; i < hardness; i++){
    int direction = rand() % 4;
    switch(direction) {
      case 0:{  //from up  to empty tile
        if(empty_row > 0 && empty_row <= 3 && prev_direction != 1){          
          #ifdef T15_USE_HITORY_REJESTR
          /*history recording must be before swap_tile, 
          because swap_tile change global variables empty_row, empty_col*/
          move_backward_rejestr.emplace_back(move_rejestr_entry_t(
            empty_row - 1, empty_col, empty_row, empty_col));
          #endif // T15_USE_HITORY_REJESTR            
          swap_tile(empty_row - 1, empty_col, empty_row, empty_col);                    
          prev_direction = direction;
        }
        else{
          i--;
        }
        break;
      }
      case 1:{  //from down
        if(empty_row < 3 && empty_row >= 0 && prev_direction != 0){
          #ifdef T15_USE_HITORY_REJESTR
          move_backward_rejestr.emplace_back(move_rejestr_entry_t(
            empty_row + 1, empty_col, empty_row, empty_col));   
          #endif // T15_USE_HITORY_REJESTR            
          swap_tile(empty_row + 1, empty_col, empty_row, empty_col);
          prev_direction = direction;       
        }
        else {
          i--;
        }
        break;
      }
      case 2:{  //from left
        if(empty_col > 0 && empty_col <= 3 && prev_direction != 3){
          #ifdef T15_USE_HITORY_REJESTR
          move_backward_rejestr.emplace_back(move_rejestr_entry_t(
            empty_row, empty_col - 1, empty_row, empty_col));
          #endif // T15_USE_HITORY_REJESTR            
          swap_tile(empty_row, empty_col - 1, empty_row, empty_col);
          prev_direction = direction;
        }
        else {
          i--;
        }
        break;
      }
      case 3:{  //from right
        if(empty_col < 3 && empty_col >= 0 && prev_direction != 2){
          #ifdef T15_USE_HITORY_REJESTR
          move_backward_rejestr.emplace_back(move_rejestr_entry_t(
            empty_row, empty_col + 1, empty_row, empty_col));
          #endif// T15_USE_HITORY_REJESTR
          swap_tile(empty_row, empty_col + 1, empty_row, empty_col);
          prev_direction = direction;          
        }
        else {
          i--;
        }
        break;
      }
    }
    #ifdef DEBUG_T15
      //create_current_state();
      //imshow(debugWindow, current_state);
      //waitKey(0);
    #endif //DEBUG_T15
  }
  is_puzzle_solved = false;
  create_current_state();
}

void reset_tiles(){
  for(int row = 0, tile_nr = 0; row < tile_height_n; ++row){
    for(int col = 0; col < tile_width_n; ++col, ++tile_nr){      
      tiles15[row][col].r = Rect(col * tile_width, row * tile_height, tile_width, tile_height);   
      
      tiles15[row][col].t = Mat(img,
        tiles15[row][col].r
      );
      tiles15[row][col].empty = false;
      tiles15[row][col].nr = tile_nr;
    }
  }
  //waitKey(0);
  current_state = Mat::zeros(img.size(), img.type());
}

int main(int argc, char **argv){
  CommandLineParser cmd_parser(argc, argv,\
  "\
  {help h || this message}\
  {@img | puzzle15p.jpg | puzzle image}\
  {@hardness | 15 | difficulty level}\
  {@language | 0 | messages language (0 = eng, 1 = pl)}\
  {@background_off | 0 | background: turn off = 1, turn on = 0}\
  ");
  if(cmd_parser.has("help")){
    cmd_parser.printMessage();
    return EXIT_SUCCESS;
  }
  try{
    img = imread(cmd_parser.get<string>("@img"), IMREAD_UNCHANGED);
    if(img.empty()){
      cerr << endl << info_6[language] << endl;      
    }
  }
  catch(cv::Exception){
    cmd_parser.printMessage();
    return EXIT_FAILURE;    
  }
  catch(...){
    cmd_parser.printMessage();
    return EXIT_FAILURE;
  }
  hardness = cmd_parser.get<int>("@hardness");  
  language = cmd_parser.get<int>("@language");
  background_off = cmd_parser.get<int>("@background_off");
  if(language != 0 && language != 1) language = 1;  

  #ifdef T15_USE_HITORY_REJESTR
  move_backward_rejestr.resize(hardness + 2);
  move_user_history.resize(hardness * 1.5);
  #endif // T15_USE_HITORY_REJESTR
  
  #ifdef DEBUG_T15
  cout << "hardness: " << hardness << endl;
  namedWindow(debugWindow);
  #endif // DEBUG_T15
  
  srand(time(NULL));
  
  //scaling image  
  resize(img, img, Size(512,512), INTER_LINEAR);

  //cutting image to the tiles    
  tile_width = img.cols / tile_width_n;
  tile_height = img.rows / tile_height_n;

  reset_tiles();  
  
  //shadowing background
  if(background_off){
    background = Mat::ones(img.size(), img.type());
    background += 200;
  }
  else {
    background = img.clone();  
  }
  background += Scalar(-100, -100, -100);
  
  create_current_state();
    
  namedWindow(gameWindowName);  
  setMouseCallback(gameWindowName, moveTile);
  int key = 0;

  //game start
  cout << endl << info_5[language] << endl;  

  while(key != 27){
    //restoring beginning state
            
    reset_tiles();  
    setWindowTitle(gameWindowName, info_1[language]);    
    
    moves_counter = 0;

    #ifdef T15_USE_HITORY_REJESTR
    //history_printed = false;
    move_user_history.clear();    
    #endif //T15_USE_HITORY_REJESTR
    
    //set empty tile
    empty_row = rand() % tile_height_n;
    empty_col = rand() % tile_width_n;    
    tiles15[empty_row][empty_col].empty = true;
    
    //prepare mess on board
    make_mess_on_board();
    
    //show board    
    imshow(gameWindowName, current_state);
    do {
      key = waitKey(0);
      #ifdef T15_USE_HITORY_REJESTR            
      if(key == 104 || key == 72) {  // 'h' 'H'
        //cout << "key: " << key << endl;
        //if(!history_printed) 
          print_moves_history();        
      }
      #endif // T15_USE_HITORY_REJESTR      
    } while(key != ' ' && key != 27);
    // if(key == ' '){
    //   key = 0;
    // }
  }

  return EXIT_SUCCESS;
}


