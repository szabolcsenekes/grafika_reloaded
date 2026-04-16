# Monkey Zoo – 3D Interactive Simulation

![C](https://img.shields.io/badge/language-C-blue)
![SDL2](https://img.shields.io/badge/library-SDL2-green)
![OpenGL](https://img.shields.io/badge/graphics-OpenGL-orange)
![Status](https://img.shields.io/badge/status-complete-success)

Egy interaktív 3D-s állatkerti szimuláció, amely **C nyelven**, **SDL2** és **OpenGL** használatával készült.  
A program célja egy dinamikus, valós idejű környezet létrehozása majmokkal, fizikai interakciókkal és látványos effektekkel.

---

## Főbb funkciók

- FPS-stílusú kamera
- Animált majmok (idle + evés)
- Fizikai alapú banándobás
- Eső + vízszimuláció
- Dinamikus köd
- Procedurálisan generált fák
- Interaktív kapuk
- Ütközéskezelés
- UI overlay

---

## Irányítás

W / A / S / D – Mozgás  
Egér – Kamera  
Shift – Futás  
Ctrl – Guggolás  
Space – Ugrás  
E – Kapu  
Q – Banán dobás  
+ / - – Fényerő  
F1 – Súgó  
ESC – Kilépés  

---

## Projekt struktúra

src/
- main.c
- camera.c/h
- scene.c/h
- renderer.c/h
- input.c/h
- model.c/h
- texture.c/h
- ui.c/h
- geom.h

---

## Fordítás

make

vagy:

gcc -Wall -Wextra -Wpedantic src/*.c -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lopengl32 -lglu32 -lm -o monkey_zoo.exe

---

## Függőségek

- SDL2  
- SDL2_image  
- OpenGL  
- GLU  

---

## Assets

assets/
- monkey.obj / monkey.png
- banana.obj / banana.png
- rock.obj / rock.png
- tree.obj / tree.png

Ezek a fájlok ([itt](https://drive.google.com/drive/folders/1pm4OsQQF1G2iEoSnMCtXJYvyyKKxh6IT?usp=drive_link)) érhetőek el

---

## Licenc

A projekt oktatási célra készült

Felhasznált külső komponens:

    - stb_easy_font -> Public Domain / MIT