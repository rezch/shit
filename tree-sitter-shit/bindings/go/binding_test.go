package tree_sitter_shit_test

import (
	"testing"

	tree_sitter "github.com/tree-sitter/go-tree-sitter"
	tree_sitter_shit "github.com/tree-sitter/tree-sitter-shit/bindings/go"
)

func TestCanLoadGrammar(t *testing.T) {
	language := tree_sitter.NewLanguage(tree_sitter_shit.Language())
	if language == nil {
		t.Errorf("Error loading Shit grammar")
	}
}
