import XCTest
import SwiftTreeSitter
import TreeSitterShit

final class TreeSitterShitTests: XCTestCase {
    func testCanLoadGrammar() throws {
        let parser = Parser()
        let language = Language(language: tree_sitter_shit())
        XCTAssertNoThrow(try parser.setLanguage(language),
                         "Error loading Shit grammar")
    }
}
