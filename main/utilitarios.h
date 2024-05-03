/** @file utilitarios.h Diversos utilitários para lidar com strings, etc.
*/

/** Função auxiliar - Verifica se um string se inicia com determinada sequência.

    @param str string que vai ser verificada.
    @param substr substring que deveria aparecer no início de str.
    
    @return true se os strings são iguais até o fim do segundo.
*/
bool str_startswith(const char *str, const char *substr);


/** Função auxiliar para decodificar uma URI.

    @param dst Destino do resultado (pode ser o mesmo que a origem).
    @param src Começa com a URL original, a ser transformada;

    Código obtido na Internet
    @see https://gist.github.com/jmsaavedra/7964251
*/
void urldecode2(char *dst, const char *src);
