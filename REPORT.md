### Project 1 

1. Overall design choices
⋅⋅⋅The very first thing we did is use the combination of exec(),fork(),and wait() to substitude the high level call system(). After that we break the command into two cases - with pipeline and without pipeline.
2. Parse the command
⋅⋅⋅To differ different cases of command, we created a function called cmd_parse(). It takes the command line as the only argument and returns a struct parse. This function would will parse the command input and break them into element we need to use later , and assign these values to struct element. And finally return the strcut.
3. sys_call()
⋅⋅⋅ sys_call is the first helper function we wrote. When we wrote the function, we simply want it to perform like the system and did not consider the case of pipeline. Therefore, this function is straight-forward. It will take the parsed struct output and the original command line, and execute the command. It will only be called when the command has no pipeline arguments. It use the structure of the combination of exec(),fork(),and wait().
4. pipeline()
⋅⋅⋅ pipeline is the helper function that will be called when there is pipeline in the command. It will take one struct parse instance as the argument. When there is pipeline in the commmand, the cmd_parse() will separate different command pipeline and store them in the element char **pipe_arr. We first created a array of struct parse in the function pipeline(). Then we use the content from the elemnt char **pipe_arr to assign value to the the array of struct parse. Then I create a loop that will keep calling fork(),dup2(),and execvp( ) until the last part of the pipeline command. Outside the loop, I used waitpid() to make sure that pipeline commands' output will present before the next prompt.
5. Redirection
⋅⋅⋅ Redirection part is simply. We just use one elemnt of struct called redir_index to check whether a redirection is needed, then execute the redirection process by using dup2(). If redir_index = 1, we need redirect the stdout. If redir_index = 2, we need to redirect the stderr. 
6. Build-in function
⋅⋅⋅ we sinmply used getcwd and chdir
7. sls
⋅⋅⋅  
