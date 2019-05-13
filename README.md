# Terminal Jeopardy!

The version of Jeopardy!™ you will die for (and can be played on the command line).
This game uses the C socket API to created a distributed TCP network across which multiple users on different computers can play.

## Getting Started

To play a game of Terminal Jeopardy! you need to do a few quick things. First, make the executables by invoking `make` or `make all` on the command line. Then, the server must be started up so that players can begin to connect. Do this by running `./server`. This should result in an output a little like:
```
Server listening on port 53651
```
That port number is important for the clients, as it is how they will connect with the server. Each person who wants to play must then run the client executable, giving as command line arguments their desired username for the game, the hostname of the computer running the server (if you don't know this off-hand, it can be obtained by invoking the command `hostname` on the machine) and the port number printed by the server. That might look something like:
```
./client Timmy hostname 53651
```
Until all the players have connected, the game will not start and each client will be told that not enough players have connected yet (the current default is 4 players, however that can be adjusted by changing the macro in `game_structs.h`). Once the required number of clients have connect, the game will begin and the board of questions will be printed in each client's terminal. From here, the game is relatively self-explanitory, starting with the player whose turn it is selecting the question for the first round.

**NOTE:** This program was developed to work on UNIX-like operating systems (Linux and MacOS) so I cannot say whether it is fully functional on Microsoft platforms.

## Authors
* **Nathan Gifford** - [enathang](https://github.com/enathang)
* **Liam Niehus-Staab** - [niehusst](https://github.com/niehusst)

## Acknowledgements
* The code in the `socket.h` file was written by [Charlie Curtsinger](https://github.com/ccurtsinger).
* The source of the JSON data containing all questions and answers used in this game is from r/datasets on [reddit](https://www.reddit.com/r/datasets/comments/1uyd0t/200000_jeopardy_questions_in_a_json_file/).
* cJSON library by Dave Gamble, provided under MIT license
* uthash library by Troy Hanson, provided under MIT license
* levenshtein library by Titus Wormer, provided under MIT license

### Dependencies (Required code is included in `deps/`)
* cJSON library for parsing game data from JSON source file
* uthash library for storing the parsed data from the JSON
* levenshtein library for fuzzy matching user answers against true answers

### Licences required for dependencies
#### cJSON
Copyright (c) 2009-2017 Dave Gamble and cJSON contributors

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#### uthash 
Copyright (c) 2005-2018, Troy D. Hanson  http://troydhanson.github.com/uthash/
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


#### levenshtein.c
(The MIT License)

Copyright (c) 2015 Titus Wormer <tituswormer@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
'Software'), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
