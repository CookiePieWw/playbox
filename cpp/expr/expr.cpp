#include <format>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

template <typename T1, typename T2> inline bool notin(T1 first, T2 second) {
	return first != second;
}

template <typename T1, typename T2, typename... Args>
inline bool notin(T1 first, T2 second, Args... others) {
	return first != second && notin(first, others...);
}

enum {
	TK_NOTYPE = 256,
	TK_EQ,
	TK_DEC,
	TK_OCT,
	TK_HEX,
	TK_NEG,
};

static struct rule {
	std::regex regex;
	int token_type;
} rules[] = {
	{std::regex(" +"), TK_NOTYPE},			   // spaces
	{std::regex("\\+"), '+'},				   // plus
	{std::regex("-"), '-'},					   // minus
	{std::regex("\\*"), '*'},				   // multiply
	{std::regex("\\/"), '/'},				   // divide
	{std::regex("\\("), '('},				   // left bracket
	{std::regex("\\)"), ')'},				   // right bracket
	{std::regex("=="), TK_EQ},				   // equal
	{std::regex("0[0-7]+"), TK_OCT},		   // octal
	{std::regex("0[xX][0-9a-fA-F]+"), TK_HEX}, // hex
	{std::regex("0|^[1-9][0-9]*"), TK_DEC},	   // decimal
};

static size_t NR_REGEX = sizeof(rules) / sizeof(rules[0]);

typedef struct token {
	int type;
	std::string str;
} Token;

static std::vector<Token> tokens;

static bool make_token(const std::string e, bool &expr_success) {
	auto position = e.begin();
	size_t i;

	while (position != e.end()) {
		std::smatch match;
		/* Try all rules one by one. */
		for (i = 0; i < NR_REGEX; i++) {
			if (std::regex_search(position, e.end(), match, rules[i].regex,
								  std::regex_constants::match_continuous)) {
				if (rules[i].token_type == TK_NOTYPE) {
					position += match[0].length();
					break;
				} else if (rules[i].token_type == '-') {
					if (tokens.size() == 0 || notin(tokens.back().type, TK_DEC,
													TK_OCT, TK_HEX, ')')) {
						rules[i].token_type = TK_NEG;
					}
				}
				tokens.emplace_back(Token{rules[i].token_type, match[0]});
				position += match[0].length();
				break;
			}
		}

		if (i == NR_REGEX) {
			expr_success = false;
			int pos = static_cast<int>(position - e.begin());
			std::cout << std::format("no match at position {}\n{}\n{:{}}^\n",
									 pos, e, "", pos);
			return false;
		}
	}
	return true;
}

static bool check_parenthese(int l, int r, bool &expr_success) {
	if (tokens[l].type != '(' || tokens[r].type != ')') {
		return false;
	}
	int i = l + 1, cnt = 1;
	while (i < r) {
		if (tokens[i].type == '(') {
			cnt++;
		} else if (tokens[i].type == ')') {
			cnt--;
		}
		if (cnt == 0) {
			return false;
		}
		i++;
	}

	if (cnt != 1) {
		expr_success = false;
		return false;
	}

	return true;
}

static int eval(int l, int r, bool &expr_success) {
	if (l > r) {
		return 0;
	} else if (l == r) {
		switch (tokens[l].type) {
		case TK_DEC:
			return std::stoi(tokens[l].str);
		case TK_OCT:
			return std::stoi(tokens[l].str, nullptr, 8);
		case TK_HEX:
			return std::stoi(tokens[l].str, nullptr, 16);
		default:
			expr_success = false;
			return 0;
		}
	} else if (check_parenthese(l, r, expr_success)) {
		return eval(l + 1, r - 1, expr_success);
	} else {
		int op_pos = l, main_op_pos = l;
		for (; op_pos <= r; ++op_pos) {
			switch (tokens[op_pos].type) {
			case '+':
			case '-':
				main_op_pos = op_pos;
				break;
			case '*':
			case '/':
				if (notin(tokens[main_op_pos].type, '+', '-')) {
					main_op_pos = op_pos;
				}
				break;
			case TK_NEG:
				if (notin(tokens[main_op_pos].type, '+', '-', '*', '/')) {
					main_op_pos = op_pos;
				}
				break;
			case '(': {
				int cnt = 1;
				op_pos++;
				while (op_pos <= r) {
					if (tokens[op_pos].type == '(') {
						cnt++;
					} else if (tokens[op_pos].type == ')') {
						cnt--;
					}
					op_pos++;
					if (cnt == 0) {
						op_pos--;
						break;
					}
				}
			}
			default:
				break;
			}
		}

		if (notin(tokens[main_op_pos].type, TK_NEG, '+', '-', '*', '/')) {
			expr_success = false;
			return 0;
		}
		int lhs = eval(l, main_op_pos - 1, expr_success);
		if (!expr_success)
			return 0;
		int rhs = eval(main_op_pos + 1, r, expr_success);
		if (!expr_success)
			return 0;

		switch (tokens[main_op_pos].type) {
		case '+':
			return lhs + rhs;
		case '-':
			return lhs - rhs;
		case '*':
			return lhs * rhs;
		case '/':
			if (rhs == 0) {
				expr_success = false;
				return 0;
			}
			return lhs / rhs;
		}
	}
	return 0;
}

int main() {
	int cnt = 0;
	while (1) {
		bool expr_success = true;
		tokens.clear();
		std::cout << std::format("(expr {}) > ", cnt);
		std::string query;

		if (!std::getline(std::cin, query) || query == "q") {
			break;
		}
		make_token(query, expr_success);
		if (!expr_success) {
			std::cout << "Invalid expression" << std::endl;
			expr_success = true;
			continue;
		}
		int result = eval(0, tokens.size() - 1, expr_success);
		if (!expr_success) {
			std::cout << "Fail to evaluate the expression" << std::endl;
			expr_success = true;
			continue;
		}
		std::cout << std::format("expr {}: {}", cnt, result) << std::endl;
		cnt++;
	}
	return 0;
}
