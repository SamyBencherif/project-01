
" Ctrl+Z to edit vim config
noremap <C-Z> :e .vimrc<CR>

" Ctrl+X to edit main.c
noremap <C-X> :e main.c<CR>:source .vimrc<CR>

" Ctrl+D to view raylib.h
noremap <C-D> :e ~/raylib/src/raylib.h<CR>

" Ctrl+Shift+D to view raylib.h in a new window
noremap <C-S-D> :vsplit<CR><C-W>l:e ~/raylib/src/raylib.h<CR>

" Ctrl+A to compile
noremap <C-A> :!clear && make<CR>

" overwrites for music
" noremap <C-X> :e tonebone.py<CR>:source .vimrc<CR>

" Ctrl+C to play tonebone song
noremap <C-C> :!python tonebone.py<CR>

" Ctrl+S to run game
noremap <C-S> :!./game &<CR><CR>

" Ctrl+E to open blender on main world
noremap <C-E> :!blender world.blend &<CR>

" Ctrl+Q to open gimp on atlas
noremap <C-Q> :!gimp atlas.png &<CR>

