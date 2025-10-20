#include "SDL2/SDL_keyboard.h"
#include "SDL2/SDL_render.h"
#include "SDL2/SDL_stdinc.h"
#include <iostream>
#include <stdexcept>
#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"
#include <array>
#include <algorithm>
#include <random>

constexpr int SCREEN_WIDTH = 480;
constexpr int SCREEN_HEIGHT = 640;

using pArrang = std::array<std::array<bool, 4>, 4>;

pArrang oArrang {{{false, true, true, false}, {false, true, true, false},
   {false, false, false, false}, {false, false, false, false}}};
pArrang lArrang{{{false, true, false, false}, {false, true, false, false},
   {false, true, true, false}, {false, false, false, false}}};
pArrang jArrang{{{false, true, false, false}, {false, true, false, false},
   {true, true, false, false}, {false, false, false, false}}};
pArrang tArrang{{{false, true, false, false}, {true, true, true, false},
   {false, false, false, false}, {false, false, false, false}}};
pArrang sArrang{{{false, true, true, false}, {true, true, false, false},
   {false, false, false, false}, {false, false, false, false}}};
pArrang zArrang{{{true, true, false, false}, {false, true, true, false},
   {false, false, false, false}, {false, false, false, false}}};
pArrang iArrang{{{false, false, false, false}, {true, true, true, true},
   {false, false, false, false}, {false, false, false, false}}};
enum class shape{
   None,
   O,
   l,
   j,
   s,
   z,
   t,
   i
};

struct point{
   int x;
   int y;
};
typedef point boardPoint;

constexpr point getBoardLocation(int x, int y){
   if (x<0 || x > 10 || y < 0 || y > 20)
      return point{0,0};
   return point {150+x*20, 150+y*20};
}

template <typename T>
class pointerCarrier {
   public:
      virtual T* get() = 0;
      virtual ~pointerCarrier() {}
};

class sdlInit {
   public:
      sdlInit () {
	 SDL_Init(SDL_INIT_VIDEO);
      }
      ~sdlInit(){
	 SDL_Quit();
      }
      sdlInit(const sdlInit&) = delete;
      sdlInit& operator=(const sdlInit&) = delete;
};

class Window : public pointerCarrier<SDL_Window>{
   public:
      Window(){
	 wind = SDL_CreateWindow("tetris", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	       SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	 if (wind == nullptr)
	    throw std::runtime_error{"Window"};
      }
      ~Window(){
	 SDL_DestroyWindow(wind);
	 wind = nullptr;
      }
      Window(const Window&) = delete;
      Window& operator=(const Window&) = delete;

      SDL_Window* get(){
	 return wind;
      }
      void update(){
	 SDL_UpdateWindowSurface(wind);
      }

   private:
      SDL_Window* wind;
};

class Surface : public pointerCarrier<SDL_Surface>{
   public:
      explicit Surface(std::string path){
	 surf = SDL_LoadBMP(path.c_str());
	 if (surf == NULL)
	    throw std::runtime_error{"Surface"};
      }
      explicit Surface(Window &win){
	 surf =SDL_GetWindowSurface(win.get());
	 if (surf == nullptr)
	    throw std::runtime_error{"Surface window"};
      }
      Surface() = delete;
      Surface(const Surface&) = delete;
      Surface& operator= (const Surface&) = delete;

      SDL_Surface * get(){
	 return surf;
      }
      ~Surface(){
	 SDL_FreeSurface(surf);
	 surf = nullptr;
      }
   private:
      SDL_Surface* surf;
};

class Texture : public pointerCarrier<SDL_Texture>{
   public:
      Texture(std::string path, pointerCarrier<SDL_Renderer> &render){
	 Surface temp(path);
	 w = temp.get()->w;
	 h = temp.get()->h;
	 text = SDL_CreateTextureFromSurface(render.get(), temp.get());
	 if (text == nullptr)
	    throw std::runtime_error{"Texture"};
      }
      ~Texture() {
	 SDL_DestroyTexture(text);
      }
      SDL_Texture * get(){
	 return text;
      }
      int width(){
	 return w;
      }
      int height(){
	 return h;
      }
   private:
      SDL_Texture* text;
      int w;
      int h;
};

class Renderer : public pointerCarrier<SDL_Renderer>{
   public:
      explicit Renderer(Window &win){
	 rend = SDL_CreateRenderer(win.get(), -1, SDL_RENDERER_ACCELERATED);	
	 if (rend == nullptr)
	    throw std::runtime_error{"Renderer"};
	 SDL_SetRenderDrawColor(rend, 0xFF, 0xFF, 0xFF, 0xFF);
      }
      ~Renderer(){
	 SDL_DestroyRenderer(rend);
      }
      Renderer(const Renderer&) = delete;
      Renderer& operator=(const Renderer&) = delete;

      SDL_Renderer* get(){
	 return rend;
      }
      void clear(Uint8 r,Uint8 g,Uint8 b,Uint8 a){
	 SDL_SetRenderDrawColor(rend, r, g, b, a);
	 SDL_RenderClear(rend);
      }
      void copy( Texture &text, point upperLeft) {
	 SDL_Rect box{upperLeft.x, upperLeft.y, text.width(), text.height()}; 
	 SDL_RenderCopy(rend, text.get(), NULL, &box);
      }
      void drawRect(point upperLeft, point lowerRight, Uint8 r,Uint8 g,Uint8 b,Uint8 a) {
	 SDL_Rect rect{upperLeft.x, upperLeft.y, lowerRight.x - upperLeft.x, lowerRight.y - upperLeft.y};
	 SDL_SetRenderDrawColor(rend, r, g, b, a);
	 SDL_RenderDrawRect(rend, &rect);
      }
      void fillRect(point upperLeft, point lowerRight, Uint8 r,Uint8 g,Uint8 b,Uint8 a) {
	 SDL_Rect rect{upperLeft.x, upperLeft.y, lowerRight.x - upperLeft.x, lowerRight.y - upperLeft.y};
	 SDL_SetRenderDrawColor(rend, r, g, b, a);
	 SDL_RenderFillRect(rend, &rect);
      }
      void present(){
	 SDL_RenderPresent(rend);
      }
   private:
      SDL_Renderer * rend;
};

class Event{};

// game logic classes here

class Piece{
   public:
      Piece(shape type, boardPoint position){
	 if (type == shape::None)
	    throw std::out_of_range{"Piece"};
	 pieceType = type;
	 boardPosition = position;
	 switch (pieceType){
	    case shape::O:
	       std::copy(oArrang.begin(), oArrang.end(), arrangement.begin());
	       break;
	    case shape::l:
	       std::copy(lArrang.begin(), lArrang.end(), arrangement.begin());
	       break;
	    case shape::j:
	       std::copy(jArrang.begin(), jArrang.end(), arrangement.begin());
	       break;
	    case shape::t:
	       std::copy(tArrang.begin(), tArrang.end(), arrangement.begin());
	       break;
	    case shape::s:
	       std::copy(sArrang.begin(), sArrang.end(), arrangement.begin());
	       break;
	    case shape::z:
	       std::copy(zArrang.begin(), zArrang.end(), arrangement.begin());
	       break;
	    case shape::i:
	       std::copy(iArrang.begin(), iArrang.end(), arrangement.begin());
	 }
      }
      std::array<std::array<bool, 4>, 4> arrange() {
	 return arrangement;	
      }
      shape type() { return pieceType;}
      boardPoint position() { return boardPosition;}
      void descend() {
	 boardPosition.y++;
      } // descend when tile below statics it
      void ascend() {
	 boardPosition.y--;
      }
      void moveRight(){
	 boardPosition.x++ ;
      }
      void moveLeft(){
	 boardPosition.x--;
      }
      void spinRight(){
	 if (pieceType == shape::O)
	    return;
	 pArrang temp;
	 if (pieceType != shape::i){
	    for (int i = 0; i < 3; i++){
	       for (int j = 0; j < 3; j++){
		  temp[i][j] = arrangement[j][i];
	       }
	    }
	    for (int i = 2; i >= 0; i--){
	       for(int j = 0; j<3; j++){
		  arrangement[2-i][j] = temp[i][j];
	       }
	    }
	 }else {
	    for (int i = 0; i < 4; i++){
	       for (int j = 0; j < 4; j++){
		  temp[i][j] = arrangement[j][i];
	       }
	    }
	    for (int i = 3; i >= 0; i--){
	       for(int j = 0; j<4; j++){
		  arrangement[3-i][j] = temp[i][j];
	       }
	    }
	 }

      }
      void spinLeft() {
	 if (pieceType == shape::O)
	    return;
	 pArrang temp;
	 if (pieceType != shape::i){
	    for (int i = 0; i < 3; i++){
	       for (int j = 0; j < 3; j++){
		  temp[i][j] = arrangement[j][i];
	       }
	    }
	    for (int j = 2; j >= 0; j--){
	       for(int i = 0; i<3; i++){
		  arrangement[i][2-j] = temp[i][j];
	       }
	    }
	 } else {
	    for (int i = 0; i < 4; i++){
	       for (int j = 0; j < 4; j++){
		  temp[i][j] = arrangement[j][i];
	       }
	    }
	    std::cout<<"notCrashed\n";
	    for (int j = 3; j >= 0; j--){
	       for(int i = 0; i<4; i++){
		  arrangement[i][3-j] = temp[i][j];
	       }
	    }
	 }
      }

   private:
      std::array<std::array<bool, 4>, 4> arrangement;
      shape pieceType;
      boardPoint boardPosition; // bottom right

};

class Board{
   public:
      Board() : boardArray() {}
      void render(Renderer & render, Texture & text) {
	 for (int i = 0; i < 20; i++){
	    for (int j = 0; j < 10; j++){
	       if (boardArray[i][j] != shape::None)
		  render.copy(text, getBoardLocation(j, i)); 

	    }
	 }
      }
      void renderPiece(Piece &toRender, Renderer &render, Texture &text) {
	 boardPoint loc = toRender.position();
	 std::array<std::array<bool, 4>, 4> arrange = toRender.arrange();
	 if (toRender.type() != shape::i){
	    for (int i = 0; i < 3; i++){
	       for (int j = 0; j < 3; j++){
		  if(arrange[i][j])
		     if (loc.y -2 + i >= 0 && loc.y -2 + i <20 && loc.x -2 + j >= 0 && loc.x -2 + j <10){
			render.copy(text, getBoardLocation(loc.x -2 + j, loc.y -2 + i)); 
		     }

	       }
	    }
	 } else{
	    for (int i = 0; i < 4; i++){
	       for (int j = 0; j < 4; j++){
		  if(arrange[i][j])
		     if (loc.y -3 + i >= 0 && loc.y -3 + i <20 && loc.x -3 + j >= 0 && loc.x -3 + j <10){
			render.copy(text, getBoardLocation(loc.x -3 + j, loc.y -3 + i)); 
		     }

	       }
	    }

	 }
      }
      void makePieceStatic(Piece &toStatic) {
	 boardPoint loc = toStatic.position();
	 std::array<std::array<bool, 4>, 4> arrange = toStatic.arrange();
	 if (toStatic.type() != shape::i){
	    for (int i = 0; i < 3; i++){
	       for (int j = 0; j < 3; j++){
		  if(arrange[i][j])
		     if (loc.y -2 + i >= 0 && loc.y -2 + i <20 && loc.x -2 + j >= 0 && loc.x -2 + j <10){
			if (boardArray[loc.y -2 + i][loc.x -2 + j] == shape::None){
			   boardArray[loc.y -2 + i][loc.x -2 + j] = toStatic.type(); 
			   std::cout << loc.x -2 + j << " " << loc.y -2 + i << "\n";
			}
		     }

	       }
	    }
	 } else{
	    for (int i = 0; i < 4; i++){
	       for (int j = 0; j < 4; j++){
		  if(arrange[i][j])
		     if (loc.y -3 + i >= 0 && loc.y -3 + i <20 && loc.x -3 + j >= 0 && loc.x -3 + j <10){
			if (boardArray[loc.y -3 + i][loc.x -3 + j] == shape::None){
			   boardArray[loc.y -3 + i][loc.x -3 + j] = toStatic.type(); 
			   std::cout << loc.x -3 + j << " " << loc.y -3 + i << "\n";
			}
		     }

	       }
	    }

	 }
	 //TODO add filled lines detection	
	 for (int i = 0; i < 20; i++){
	    fullLines[i] = true;
	    for (int j = 0;  j < 10; j++){
	       if (boardArray[i][j] == shape::None)
		  fullLines[i] = false;
	    }
	    if (fullLines[i])
	       std::cout << "Full line on: " << i << "\n";
	 }
      }
      bool checkPieceColliding (Piece colliding) {
	 boardPoint loc = colliding.position();
	 std::array<std::array<bool, 4>, 4> arrange = colliding.arrange();
	 if (colliding.type() != shape::i){
	    for (int i = 0; i < 3; i++){
	       for (int j = 0; j < 3; j++){
		  boardPoint location {loc.x -2+j, loc.y-2+i};
		  if(arrange[i][j]){
		     if (!(loc.y -2 + i >= 0 && loc.y -2 + i <20 && loc.x -2 + j >= 0 && loc.x -2 + j <10)){
			std::cout << "barrier collide\n";
			return true;	      
		     }
		     if (boardArray[location.y][location.x] != shape::None){
			std::cout << "piece collide\n";
			std::cout << location.x << ", " << location.y << "\n" << static_cast <int>(boardArray[location.y][location.x]) << "\n\n";
		     return true;
		     }

		  }


	       }
	    }
	 } else{
	    for (int i = 0; i < 4; i++){
	       for (int j = 0; j < 4; j++){
		  boardPoint location {loc.x -3+j, loc.y-3+i};
		  if(arrange[i][j]){
		     if (!(loc.y -3 + i >= 0 && loc.y -3 + i <20 && loc.x -3 + j >= 0 && loc.x -3 + j <10)){
			std::cout << "barrier collide\n";
			return true;
		     }
		     if (boardArray[location.y][location.x] != shape::None){
			std::cout << "piece colllide\n";
			std::cout << location.x << ", " << location.y << ", : " << static_cast<int>(boardArray[location.y][location.x]) << "\n\n";
			return true;
		     }
		  }

	       }
	    }

	 }
	 return false;
      }
      void clearFullLines(){
	 bool quit = false;
	 while (!quit){
	 for (int i = 0; i < 20; i++){
	    if (fullLines[i]){
	       std::cout << "full line\n\n" << i << "\n";
	       std::copy (boardArray.begin(), boardArray.begin() + i, boardArray.begin() +1);
	       std::array<shape, 10> temp {shape::None,shape::None,shape::None,shape::None,shape::None,shape::None,shape::None,shape::None,shape::None,shape::None};
	       boardArray[0] = std::move(temp);
	       std::copy(fullLines.begin(), fullLines.begin() + i, fullLines.begin() +1);
	       fullLines[0] = false;
	       break;
	    }
	 }
	 quit = true;
	 for (auto & ind : fullLines){
	    if (ind == true)
	       
	       quit = false;
	 }
	 }
      }

   private:
      std::array<std::array<shape, 10>, 20> boardArray;
      std::array<bool, 20> fullLines;
};
class PieceQueue{
public:
   PieceQueue() : rd(), gen(rd()), distr(1,7){
      for (auto & pie : Pieces){
	 pie = new Piece(static_cast<shape>(distr(gen)), boardPoint{5,4});
      }
   }
   Piece* getPiece() {
      Piece* temp = Pieces[0];
      std::copy(Pieces.begin() +1, Pieces.end(), Pieces.begin());
      Pieces[2] = new Piece(static_cast<shape>(distr(gen)), boardPoint{5,4});
      std::cout << "new piece\n";
      return temp;
   }


private:
   // don't have time to learn much about these.
   std::random_device rd;
   std::mt19937 gen;
   std::uniform_int_distribution<> distr;

   std::array<Piece*, 3> Pieces;

   
};

// important now
class TimeController{
public:
   TimeController (PieceQueue& pq, Board & b) : board(b), queue(pq) {
      std::cout << "got piece\n";
      currentPiece = queue.getPiece();
   }
   Piece &getPiece(){
      return *currentPiece;
   }
   bool Step(){
      if (currentPiece == nullptr)
	 return true;
      if (const Uint32 thisTime = SDL_GetTicks(); thisTime - lastTime >= tickWait){
	 lastTime = thisTime;
	 currentPiece->descend();
	 if (board.checkPieceColliding(*currentPiece)){
	    currentPiece->ascend();
	    board.makePieceStatic(*currentPiece);
	    delete currentPiece;
	    currentPiece = nullptr;
	    std::cout << "landed\n";
	    board.clearFullLines();
	    currentPiece = queue.getPiece();
	    if (board.checkPieceColliding(*currentPiece))
	       return true;
	 }
      }
      return false;

   }
   void Left() {
      if (currentPiece == nullptr)
	 return;
      if (const Uint32 thisTime = SDL_GetTicks(); thisTime - lastMove >= moveWait){
	 lastMove = thisTime;
	 currentPiece->moveLeft();
	 if (board.checkPieceColliding(*currentPiece)){
	    currentPiece->moveRight();  
	 }
      }
   }
   void Right() {
      if (currentPiece == nullptr)
	 return;
      if (const Uint32 thisTime = SDL_GetTicks(); thisTime - lastMove >= moveWait){
	 lastMove = thisTime;
	 currentPiece->moveRight();
	 if (board.checkPieceColliding(*currentPiece)){
	    currentPiece->moveLeft();  
	 }
      }
   }
   void Spin() {
      if (currentPiece == nullptr)
	 return;
      if (const Uint32 thisTime = SDL_GetTicks(); thisTime - lastSpin >= spinWait){
	 lastSpin = thisTime;
	 currentPiece->spinRight();
	 if (board.checkPieceColliding(*currentPiece)){
	    currentPiece->spinLeft();
	 }
      }
   }

private:
   const Uint32 tickWait = 300;
   Uint32 lastTime;

   const Uint32 moveWait = 200;
   Uint32 lastMove;
   
   const Uint32 spinWait = 200;
   Uint32 lastSpin;

   Piece * currentPiece;
   Board &board;
   PieceQueue& queue;

};




int main () {
   sdlInit sdl;
   Window window;
   Renderer renderer(window);
   Texture text("Images/piece.bmp", renderer);
   Board board;
   PieceQueue pq;
   TimeController tControl(pq, board);

   SDL_Event e;
   bool quit {false};
   while (!quit){
      while (SDL_PollEvent(&e)){
	 if (e.type == SDL_QUIT)
	    quit = true;
      }
      const Uint8 * keyState = SDL_GetKeyboardState( NULL);
      if (keyState[ SDL_SCANCODE_UP ] )
	 tControl.Spin();
      if (keyState[ SDL_SCANCODE_LEFT ] )
	 tControl.Left();
      else if (keyState[ SDL_SCANCODE_RIGHT ] )
	 tControl.Right();
      renderer.clear(0xFF, 0xFF, 0xFF, 0xFF);
      renderer.drawRect(getBoardLocation(0,0), getBoardLocation(10, 20), 0x0, 0x0, 0x0, 0xFF);
      board.render(renderer, text);
      quit = quit ? true : tControl.Step();
      board.renderPiece(tControl.getPiece(), renderer, text);
      renderer.present();

   }
}

