#pragma once
#include <string>
#include <functional>
#include <variant>
#include <memory>
#include <vector>
#include "../ast.h"
#include "Environment.h"
#define OBJECT_H

namespace Object {

	extern const std::string STRING_OBJ_ = "STRING";
	extern const std::string INTEGER_OBJ_ = "INTEGER";
	extern const std::string BOOLEAN_OBJ_ = "BOOLEAN";
	extern const std::string NIL_OBJ_ = "Nil";
	extern const std::string RETURN_VALUE_OBJ = "RETURN_VALUE";
	extern const std::string ERROR_OBJ_ = "ERROR";
	extern const std::string FUNCTION_OBJ_ = "FUNCTION";
	extern const std::string BUILTIN_OBJ_ = "BUILTIN";
	extern const std::string ARRAY_OBJ_ = "ARRAY";
	extern const std::string HASH_OBJ_ = "HASH";

	typedef std::string ObjectType;

	class HashKey {
	public:
		HashKey() {}
		HashKey(ObjectType type, uint64_t value) :Type(type), Value(value) {}
		ObjectType Type;
		uint64_t Value;
	};


	class Object {
	public:
		virtual ObjectType Type(void) const = 0;
		virtual std::string Inspect(void)const = 0;
	};
	typedef std::function<Object(std::vector<Object>)> BuiltinFunction;

	class Hashable {
	public:
		virtual HashKey hashKey(void) = 0;
	};


	class Builtin :Object {
	public:
		BuiltinFunction Fn;

		ObjectType Type(void) const override {
			return BUILTIN_OBJ_;
		};

		std::string Inspect(void) const override {
			return "builtin function";
		};

	};

	// 64 bit integer
	class I64 : Object, Hashable {
	public:
		int64_t Value;
		ObjectType Type(void) const override {
			return INTEGER_OBJ_;
		};

		std::string Inspect(void) const override {
			return std::to_string(Value);
		};

		HashKey hashKey()override {
			return HashKey(Type(), static_cast<uint64_t>(Value));
		}
	};

	class Boolean : Object, Hashable {
	public:
		bool Value;
		ObjectType Type(void) const override {
			return BOOLEAN_OBJ_;
		};

		std::string Inspect(void) const override {
			return std::to_string(Value);
		};

		HashKey hashKey()override {
			uint64_t value;
			if (Value)
			{
				value = 1;
			}
			else {
				value = 0;
			}
			;
			return HashKey(Type(), value);
		}
	};

	class Nil :Object {
	public:
		ObjectType Type(void) const override {
			return NIL_OBJ_;
		};

		std::string Inspect(void) const override {
			return "nil";
		};
	};

	class Error :Object {
	public:
		std::string Message;
		ObjectType Type(void) const override {
			return ERROR_OBJ_;
		};

		std::string Inspect(void) const override {
			return "Error: " + Message;
		};
	};

	class String :Object, Hashable {
	public:
		std::string Value;
		ObjectType Type(void) const override {
			return STRING_OBJ_;
		};

		std::string Inspect(void) const override {
			return Value;
		};
		HashKey hashKey()override {
			std::hash<std::string> hash;
			auto h = hash(Value);
			return HashKey(Type(), h);
		}
	};

	class Array :Object {
	public:
		std::vector<Object> Elements;
		ObjectType Type(void) const override {
			return ARRAY_OBJ_;
		};

		std::string Inspect(void) const override {
			std::string out;
			auto elements = vector<std::string>();
			for (auto& el : Elements) {
				elements.push_back(el.Inspect());
			}
			out += "[";
			for (auto& str : elements) {
				out += ", ";
				out += str;
			}
			out += "]";
			return out;
		};
	};


	
	class Function :Object {
	public:
		std::vector < std::unique_ptr<Identifier>>Parameters;
		std::unique_ptr<BlockStatement> Body;
		std::unique_ptr<Environment> Env;


		std::string Message;
		ObjectType Type(void) const override {
			return FUNCTION_OBJ_;
		};

		std::string Inspect(void) const override {
			std::string out;
			auto params = vector<std::string>();
			for (auto& p : std::move(Parameters)) {
				params.push_back(p->String());
			}
			out += "func(";
			for (auto& str : params) {
				out += ", ";
				out += str;
			}
			out += "){\n";
			out += Body->String() + "\n";
			return out;
		}


	};

	class ReturnValue :Object {
	public:
		std::unique_ptr<Object> Value;


		ObjectType Type(void) const override {
			return RETURN_VALUE_OBJ;
		};

		std::string Inspect(void) const override {
			return Value->Inspect();
		};

	};

	class HashPair {
	public:
		std::variant<Builtin, Function, String, Boolean, Array, Hash, I64, ReturnValue, Nil> Key;
		std::variant<Builtin, Function, String, Boolean, Array, Hash, I64, ReturnValue, Nil> Value;
	};

	class Hash :Object {
	public:
		std::map<HashKey, HashPair> Pairs;
		ObjectType Type(void) const override {
			return HASH_OBJ_;
		};

		std::string Inspect(void) const override {
			std::string out;
			auto pairs = vector<std::string>();
			for (auto& p : Pairs) {
				using var_t = std::variant<Builtin, Function, String, Boolean, Array, Hash, I64, ReturnValue, Nil>;
				auto str1 = std::visit([out](auto&& arg) -> var_t { out += arg.Inspect() += ": "; }, p.second.Key);
				auto str2 = std::visit([out](auto&& arg) -> var_t { out += arg.Value; }, p.second.Value);
			}
			out += "{";
			for (auto& str : pairs) {
				out += ", ";
				out += str;
			}
			out += "}";
			return out;
		};
	};

};