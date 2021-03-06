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
#define T15_USE_HISTORY_REJESTR
#define T15_TILE_HIGHLIGHT

#ifdef DEBUG_T15
#define PDEBUG(X) cout << (X) << endl;
#else
#define PDEBUG(X) ;
#endif

//namespace my_app {
  const int tile_width_n = 4;
  const int tile_height_n = 4;
  //const int tile_size = tile_width_n * tile_height_n;

  struct tile_t{
    Mat t;
    Rect r;
    bool empty;
    int nr;     //original number (for checking if solved)
  } tiles15[tile_height_n][tile_width_n];

  Mat image;  //original image
  Mat img;    //tiles - clone of image
  Mat background; //background - clone of image
  Mat current_state; //wisible state of puzzle
  Mat tmp;
  Point move_start, move_end, move_over;
  int tile_width;
  int tile_height;
  int empty_row;
  int empty_col;  

  #ifdef T15_USE_HISTORY_REJESTR
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
  #endif // T15_USE_HISTORY_REJESTR  

  int moves_counter;
  const int border_color_start = 1;
  const int border_color_end = 2;
  const int  border_color_hide = 3;
  
  int hardness;     //hardness degree
  int background_off = 0;
  int language = 0;  //0 - eng, 1 - pl  
  string info_1[] = {
    "[space] -> again, [esc] -> end  | move: ",
    "[spacja] -> od nowa [esc] -> koniec |Ruch:"
  };
  string info_2[] = {
  #ifdef T15_USE_HISTORY_REJESTR
    "PUZZLE SOLVED  ([space] - again, [h] - show history, [esc] - end)",
    "UKLADANKA U??OZONA ([spacja] -> jesze raz, [h] - historia, [esc] -> zakoncz)"  
  #else
    "PUZZLE SOLVED  ([space] - again, [esc] - end)",
    "UKLADANKA U??OZONA ([spacja] -> jesze raz, [esc] -> zakoncz)"    
  #endif // T15_USE_HISTORY_REJESTR
  };  
  string info_3[] = {
    "can't make a move over several fields",
    "nie mozna wykonac ruchu przez kilka p??l"
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
  
  #ifdef T15_USE_HISTORY_REJESTR
  string info_8[] = {
    "history of user moves: ",
    "historia ruch??w u??ytkownika: "
  };
  string info_9[] = {
    "reverse history of move during preparing mess:",
    "odwotna historia ruch??w podczas mieszania fragment??w uk??adanki:"
  };
  //bool history_printed = false;
  #endif //T15_USE_HISTORY_REJESTR  
  
  string gameWindowName = "puzzle15";
  bool is_puzzle_solved;
  
  #ifdef DEBUG_T15
  string debugWindow = "debugWindow";
  #endif // DEBUG_T15
//}

void moveTile(int event, int x, int y, int flags, void* userdata);

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
  //PDEBUG("create_current_state()");
  pasteMat2Mat(
    background, 
    Rect(0,0,image.rows, image.cols), 
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


#ifdef T15_TILE_HIGHLIGHT
Scalar color_start = Scalar(255,0,0);
Scalar color_end = Scalar(0,0,255); //end, warning
Scalar color_neutral = Scalar(200,100,100);
int line_width = 4;
bool tile_highlight_on = true;


inline void draw_highlight(tile_t& t, Scalar& color_) {
  //PDEBUG("draw_highlight");
  rectangle(
    t.t,     
    Point( 
      0 + line_width
      , 
      0 + line_width      
      ),
    Point(      
      0 + t.r.height - line_width      
      ,       
      0 + t.r.width - line_width
      ),
    color_, line_width,8,0);   
}


void tile_border(int r, int c, int color){
  //PDEBUG("tile_border");  
  if(color == border_color_start) {
    //cout << "click DOWN tile : [row,col] : [" << r << "," << c << "]" << endl;                 
    draw_highlight(tiles15[r][c],color_start);     
  }
  if(color == border_color_end) {
    /*
    draws a red border on tiles that you cannot move to (warning)
    On the allowed tile it also draws, but it is not visible, 
    so you can not see the red frame.
    */
    //cout << "click UP tile : [row,col] : [" << r << "," << c << "]" << endl;      
    draw_highlight(tiles15[r][c],color_end); 
  }
  if(color == border_color_hide){
    //cout << "click OVER tile : [row,col] : [" << r << "," << c << "]" << endl;     
    draw_highlight(tiles15[r][c],color_neutral); 
  }  
  create_current_state();
  imshow(gameWindowName, current_state);
}
#endif // T15_TILE_HIGHLIGHT


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


#ifdef T15_USE_HISTORY_REJESTR
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
#endif// T15_USE_HISTORY_REJESTR


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


bool start_lock = false;
bool end_lock = false;
void moveTile(int event, int x, int y, int flags, void* userdata){
  //PDEBUG("moveTile");
  if(is_puzzle_solved) 
    return;
  switch(event){
    case EVENT_LBUTTONDOWN:{

      move_start = Point(x,y);    
      move_over = Point(x,y);
      start_lock = true;
      PDEBUG("moveTile - EVENT_LBUTTONDOWN");
      #ifdef T15_TILE_HIGHLIGHT
      if(tile_highlight_on)
        tile_border(move_start.y / tile_height, move_start.x / tile_width, border_color_start);
      #endif// T15_TILE_HIGHLIGHT          
      
      break;
    }
    case EVENT_MOUSEMOVE:{
     if(start_lock && !end_lock){    
        PDEBUG("moveTile - EVENT_MOUSEMOVE");
        #ifdef T15_TILE_HIGHLIGHT
        if(tile_highlight_on)
          if(move_over != move_start)
            tile_border(move_over.y / tile_height, move_over.x / tile_width, border_color_hide);
        #endif // T15_TILE_HIGHLIGHT
        
        move_over = Point(x,y);
        
        #ifdef T15_TILE_HIGHLIGHT
        if(tile_highlight_on)
          tile_border(move_over.y / tile_height, move_over.x / tile_width, border_color_end);
        #endif // T15_TILE_HIGHLIGHT    
      }
      break;
    }
    case EVENT_LBUTTONUP: {

      if(start_lock  && !end_lock){    
        PDEBUG("moveTile - EVENT_LBUTTONUP");
        end_lock = true;
        move_end = Point(x,y);
        
        #ifdef T15_TILE_HIGHLIGHT
        if(tile_highlight_on){
          tile_border(move_over.y / tile_height, move_over.x / tile_width, border_color_hide);
          tile_border(move_start.y / tile_height, move_start.x / tile_width, border_color_hide);
          tile_border(move_end.y / tile_height, move_end.x / tile_width, border_color_hide);
        }
        #endif // T15_TILE_HIGHLIGHT

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
            moves_counter++;      
            swap_tile(row_start, col_start, row_end, col_end);
            #ifdef T15_USE_HISTORY_REJESTR
            move_user_history.emplace_back(move_rejestr_entry_t(row_start, col_start, row_end, col_end));
            #endif // T15_USE_HISTORY_REJESTR
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
        start_lock = false;
        end_lock = false;  
      }       
      break;
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
  #ifdef T15_USE_HISTORY_REJESTR
  move_backward_rejestr.clear();
  #endif // T15_USE_HISTORY_REJESTR  
  int prev_direction = 5;
  for(int i = 0; i < hardness; i++){
    int direction = rand() % 4;
    switch(direction) {
      case 0:{  //from up  to empty tile
        if(empty_row > 0 && empty_row <= 3 && prev_direction != 1){          
          #ifdef T15_USE_HISTORY_REJESTR
          /*history recording must be before swap_tile, 
          because swap_tile change global variables empty_row, empty_col*/
          move_backward_rejestr.emplace_back(move_rejestr_entry_t(
            empty_row - 1, empty_col, empty_row, empty_col));
          #endif // T15_USE_HISTORY_REJESTR            
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
          #ifdef T15_USE_HISTORY_REJESTR
          move_backward_rejestr.emplace_back(move_rejestr_entry_t(
            empty_row + 1, empty_col, empty_row, empty_col));   
          #endif // T15_USE_HISTORY_REJESTR            
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
          #ifdef T15_USE_HISTORY_REJESTR
          move_backward_rejestr.emplace_back(move_rejestr_entry_t(
            empty_row, empty_col - 1, empty_row, empty_col));
          #endif // T15_USE_HISTORY_REJESTR            
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
          #ifdef T15_USE_HISTORY_REJESTR
          move_backward_rejestr.emplace_back(move_rejestr_entry_t(
            empty_row, empty_col + 1, empty_row, empty_col));
          #endif// T15_USE_HISTORY_REJESTR
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
  img = image.clone();
  for(int row = 0, tile_nr = 0; row < tile_height_n; ++row){
    for(int col = 0; col < tile_width_n; ++col, ++tile_nr){      
      tiles15[row][col].r = Rect(col * tile_width, row * tile_height, tile_width, tile_height);   
      
      tiles15[row][col].t = Mat(img,
        tiles15[row][col].r
      );

      #ifdef T15_TILE_HIGHLIGHT
      if(tile_highlight_on){
        draw_highlight(
          tiles15[row][col],
          color_neutral
        );
      }     
      #endif // T15_TILE_HIGHLIGHT 

      tiles15[row][col].empty = false;
      tiles15[row][col].nr = tile_nr;
    }
  }
  //waitKey(0);
  current_state = Mat::zeros(image.size(), image.type());
}


int main(int argc, char **argv){
  CommandLineParser cmd_parser(argc, argv,
  #ifdef T15_TILE_HIGHLIGHT
  "\
  {help h || this message}\
  {@img | puzzle15p.jpg | puzzle image}\
  {@hardness | 15 | difficulty level}\
  {@language | 0 | messages language (0 = eng, 1 = pl)}\
  {@background_off | 0 | background: turn off = 1, turn on = 0}\
  {@highlight_tile | 0 | turn on/off tile highlight (on = 1, off = 0)}\
  ");
  #else
  "\
  {help h || this message}\
  {@img | puzzle15p.jpg | puzzle image}\
  {@hardness | 15 | difficulty level}\
  {@language | 0 | messages language (0 = eng, 1 = pl)}\
  {@background_off | 0 | background: turn off = 1, turn on = 0}\
  ");
  #endif // T15_TILE_HIGHLIGHT
  if(cmd_parser.has("help")){
    cmd_parser.printMessage();
    return EXIT_SUCCESS;
  }
  try{
    image = imread(cmd_parser.get<string>("@img"), IMREAD_UNCHANGED);
    if(image.empty()){
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

  #ifdef T15_USE_HISTORY_REJESTR
  move_backward_rejestr.resize(hardness + 2);
  move_user_history.resize(hardness * 1.5);
  #endif // T15_USE_HISTORY_REJESTR
  
  #ifdef T15_TILE_HIGHLIGHT
  tile_highlight_on = cmd_parser.get<bool>("@highlight_tile");
  #endif //T15_TILE_HIGHLIGHT

  #ifdef DEBUG_T15
  cout << "hardness: " << hardness << endl;
  namedWindow(debugWindow);
  #endif // DEBUG_T15
  
  srand(time(NULL));
  
  //scaling image  
  resize(image, image, Size(512,512), INTER_LINEAR);

  //cutting image to the tiles    
  tile_width = image.cols / tile_width_n;
  tile_height = image.rows / tile_height_n;

  reset_tiles();  
  
  //shadowing background
  if(background_off){
    background = Mat::ones(image.size(), image.type());
    background += 200;
  }
  else {
    background = image.clone();  
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

    #ifdef T15_USE_HISTORY_REJESTR
    //history_printed = false;
    move_user_history.clear();    
    #endif //T15_USE_HISTORY_REJESTR
    
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
      #ifdef T15_USE_HISTORY_REJESTR            
      if(key == 104 || key == 72) {  // 'h' 'H'
        //cout << "key: " << key << endl;
        //if(!history_printed) 
          print_moves_history();        
      }
      #endif // T15_USE_HISTORY_REJESTR      
    } while(key != ' ' && key != 27);
    // if(key == ' '){
    //   key = 0;
    // }
  }

  return EXIT_SUCCESS;
}


