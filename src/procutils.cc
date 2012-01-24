#include <errno.h>
#include <iostream>
#include <node.h>
#include <string.h>
#include <unistd.h>
#include <v8.h>

#ifdef __linux__
#include <pty.h>
#else
#include <util.h>
#endif // __linux__

v8::Handle<v8::Value> ForkPty(const v8::Arguments& args) {
    v8::HandleScope scope;
    int amaster;
    pid_t pid = forkpty(&amaster, NULL, NULL, NULL);
    if (pid == -1) {
        v8::Handle<v8::String> message = v8::String::Concat(
            v8::String::New("Error calling forkpty: "),
            v8::String::New(strerror(errno))
        );
        return v8::ThrowException(message);
    }

    v8::Handle<v8::Array> result = v8::Array::New(2);
    result->Set(0, v8::Integer::New(pid));
    result->Set(1, v8::Integer::New(amaster));
    return scope.Close(result);
}

v8::Handle<v8::Value> Execvp(const v8::Arguments& args) {
    v8::HandleScope scope;

    if (!args[0]->IsString()) {
        return v8::ThrowException(v8::String::New("file must be a string"));
    }

    if (!args[1]->IsArray()) {
        return v8::ThrowException(v8::String::New("argv must be an array"));
    }

    v8::String::Utf8Value execFile(args[0]);
    v8::Handle<v8::Array> execArgs = v8::Handle<v8::Array>::Cast(args[1]);

    // Convert execArgs to an array of C-strings, with a NULL at the end.
    int length = execArgs->Length();
    char *argv[execArgs->Length() + 1];
    for (int i = 0; i < length; i++) {
        v8::String::Utf8Value argValue(execArgs->Get(i)->ToString());
        argv[i] = strdup(*argValue);
    }
    argv[length] = NULL;

    // execvp returns only if an error occurred. In that case, the return
    // value is -1.
    execvp(*execFile, argv);

    v8::Handle<v8::String> message = v8::String::Concat(
        v8::String::New("Error calling execvp: "),
        v8::String::New(strerror(errno))
    );
    return v8::ThrowException(message);
}

void init(v8::Handle<v8::Object> target) {
    target->Set(v8::String::NewSymbol("forkpty"), v8::FunctionTemplate::New(ForkPty)->GetFunction());
    target->Set(v8::String::NewSymbol("execvp"), v8::FunctionTemplate::New(Execvp)->GetFunction());
}

NODE_MODULE(procutils, init)
