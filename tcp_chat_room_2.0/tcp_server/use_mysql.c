#include <mysql.h>
#include <my_global.h>

void mysql_connect(MYSQL *mysql, char *databaseName) {
    if (mysql_real_connect(mysql, "localhost", "test", "test", databaseName, 0, NULL, 0) == NULL) {
        printf("mysql connect error, %s\n", mysql_error(mysql));
    }
}

void mysql_crud(MYSQL *mysql, char *instruction) {
    if (mysql_real_query(mysql, instruction, (unsigned int)strlen(instruction)) != 0) {  //说明该用户曾经注册
        printf("mysql insert error, %s\n", mysql_error(mysql));
    }
}
