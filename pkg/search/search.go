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

// Package search implements various functions used to search a
// position for the best move.
package search

//go:generate go run laptudirm.com/x/mess/internal/generator/reductions

import (
	"errors"
	realtime "time"

	"laptudirm.com/x/mess/pkg/board"
	"laptudirm.com/x/mess/pkg/board/move"
	"laptudirm.com/x/mess/pkg/board/piece"
	"laptudirm.com/x/mess/pkg/board/square"
	"laptudirm.com/x/mess/pkg/search/eval"
	"laptudirm.com/x/mess/pkg/search/eval/pesto"
	"laptudirm.com/x/mess/pkg/search/time"
	"laptudirm.com/x/mess/pkg/search/tt"
)

// maximum depth to search to
const MaxDepth = 256

// NewContext creates a new search Context.
func NewContext(reporter Reporter, ttSize int) *Context {
	evaluator := &pesto.EfficientlyUpdatable{}

	return &Context{
		// default position
		board: board.New(board.EU(evaluator), board.FEN(board.StartFEN)),

		evaluator: evaluator,

		tt:      tt.NewTable(ttSize),
		stopped: true,

		reporter: reporter,
	}
}

// Context stores various options, state, and debug variables regarding a
// particular search. During multiple searches on the same position, the
// internal board (*Context).Board should be switched out, while a brand
// new Context should be used for different games.
type Context struct {
	// search state
	board      *board.Board
	sideToMove piece.Color
	tt         *tt.Table
	stopped    bool

	evaluator eval.EfficientlyUpdatable

	// principal variation
	pv      move.Variation
	pvScore eval.Eval

	// stats
	stats    Stats
	reporter Reporter

	// search limits
	time   time.Manager
	limits Limits

	// move ordering stuff
	history [piece.ColorN][square.N][square.N]eval.Move
	killers [MaxDepth][2]move.Move
}

// Search initializes the context for a new search and calls the main
// iterative deepening function. It checks if the position is illegal
// and cleans up the context after the search finishes.
func (search *Context) Search(limits Limits) (move.Variation, eval.Eval, error) {

	search.start(limits)
	defer search.Stop()

	// illegal position check; king can be captured
	if search.board.IsInCheck(search.board.SideToMove.Other()) {
		return move.Variation{}, eval.Inf, errors.New("search move: position is illegal")
	}

	pv, eval := search.iterativeDeepening()
	return pv, eval, nil
}

// InProgress reports whether a search is in progress on the given context.
func (search *Context) InProgress() bool {
	return !search.stopped
}

// ResizeTT resizes the search's transposition table.
func (search *Context) ResizeTT(mbs int) {
	search.tt.Resize(mbs)
}

// Stop stops any ongoing search on the given context. The main search
// function will immediately return after this function is called.
func (search *Context) Stop() {
	search.stopped = true
}

// start initializes search variables during the start of a search.
func (search *Context) start(limits Limits) {
	// reset principal variation
	search.pv.Clear()

	// reset stats
	search.stats = Stats{}
	search.sideToMove = search.board.SideToMove

	// age the transposition table
	search.tt.NextEpoch()

	// start search
	search.UpdateLimits(limits)
	search.stopped = false // search not stopped

	// start search timer
	search.stats.SearchStart = realtime.Now()
}
