#pragma once
#ifndef Environment_h
#define Environment_h

#include <map>
#include <memory>
#include <string>

#include "llvm/IR/Value.h"

/**
 * Environment: names storage.
 */
class Environment : public std::enable_shared_from_this<Environment> {
public:
    /**
     * Creates an environment with the given record.
     */
    Environment(std::map<std::string, llvm::Value*> record,
        std::shared_ptr<Environment> parent)
        : record_(record), parent_(parent) {}

    /**
     * Creates a variable with the given name and value.
     */
    llvm::Value* define(const std::string& name, llvm::Value* value) {
        record_[name] = value;
        return value;
    }

    /**
     * Returns the value of a defined variable, or throws
     * if the variable is not defined.
     */
    llvm::Value* lookup(const std::string& name) {
        return resolve(name);
    }

private:
    /**
     * Returns specific environment in which a variable is defined, or
     * throws if a variable is not defined.
     */
    llvm::Value* resolve(const std::string& name) {
        auto obj = record_.find(name);
        if (obj == record_.end() && parent_!=nullptr)
        {
            auto obj_ = parent_->lookup(name);
            obj->second = obj_;
        }
        return obj->second;
    }

    /**
     * Bindings storage
     */
    std::map<std::string, llvm::Value*> record_;

    /**
     * Parent link
     */
    std::shared_ptr<Environment> parent_;
};

#endif