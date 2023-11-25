3150 # Context switch
3151 #
3152 #   void swtch(struct context **old, struct context *new);
3153 #
3154 # Save the current registers on the stack, creating
3155 # a struct context, and save its address in *old.
3156 # Switch stacks to new and pop previously-saved registers.
3157 
3158 .globl swtch
3159 swtch:
3160   movl 4(%esp), %eax
3161   movl 8(%esp), %edx
3162 
3163   # Save old callee-saved registers
3164   pushl %ebp
3165   pushl %ebx
3166   pushl %esi
3167   pushl %edi
3168 
3169   # Switch stacks
3170   movl %esp, (%eax)
3171   movl %edx, %esp
3172 
3173   # Load new callee-saved registers
3174   popl %edi
3175   popl %esi
3176   popl %ebx
3177   popl %ebp
3178   ret
3179 
3180 
3181 
3182 
3183 
3184 
3185 
3186 
3187 
3188 
3189 
3190 
3191 
3192 
3193 
3194 
3195 
3196 
3197 
3198 
3199 
