// Copyright © 2023 Rak Laptudirm <rak@laptudirm.com>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package search

import (
	"laptudirm.com/x/mess/pkg/board"
	"laptudirm.com/x/mess/pkg/board/move"
	"laptudirm.com/x/mess/pkg/formats/fen"
)

// String returns a human-readable ascii art representation of the search
// board, along with it's fen string and zobrist hash.
func (search *Context) String() string {
	return search.board.String()
}

// UpdatePosition updates the search board with the given fen.
func (search *Context) UpdatePosition(fen fen.String) {
	search.board.UpdateWithFEN(fen)
	search.tt.Clear()
}

func (search *Context) Board() *board.Board {
	return search.board
}

func (search *Context) MakeMove(m move.Move) {
	search.board.MakeMove(m)
}

// MakeMoves makes the given moves on the search board.
func (search *Context) MakeMoves(moves ...string) {
	for _, m := range moves {
		search.board.MakeMove(search.board.NewMoveFromString(m))
	}
}
