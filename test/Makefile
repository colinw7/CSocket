CC = g++
RM = rm

CDEBUG = -g
LDEBUG = -g

INC_DIR = ../include
OBJ_DIR = .
LIB_DIR = ../lib
BIN_DIR = ../bin

all: $(BIN_DIR)/CSocketINetClient $(BIN_DIR)/CSocketINetServer $(BIN_DIR)/CSocketUnixClient $(BIN_DIR)/CSocketUnixServer

CPPFLAGS = \
-std=c++14 \
-I$(INC_DIR) \
-I../../CReadLine/include \
-I../../CFile/include \
-I../../CFileUtil/include \
-I../../CStrUtil/include \
-I../../CUtil/include \
-I../../COS/include \
-I.

LIBS = \
-L$(LIB_DIR) \
-L../../CReadLine/lib \
-L../../CFile/lib \
-L../../CFileUtil/lib \
-L../../CStrUtil/lib \
-L../../CRegExp/lib \
-L../../COS/lib \
-lCSocket \
-lCReadLine \
-lCFileUtil \
-lCFile \
-lCStrUtil \
-lCRegExp \
-lCOS \
-lreadline -ltre

clean:
	$(RM) -f $(OBJ_DIR)/*.o
	$(RM) -f $(BIN_DIR)/CSocketTest

.SUFFIXES: .cpp

.cpp.o:
	$(CC) -c $< -o $(OBJ_DIR)/$*.o $(CPPFLAGS)

$(BIN_DIR)/CSocketINetClient: CSocketINetClient.o $(LIB_DIR)/libCSocket.a
	$(CC) $(LDEBUG) -o $(BIN_DIR)/CSocketTest CSocketINetClient.o $(LIBS)

$(BIN_DIR)/CSocketINetServer: CSocketINetServer.o $(LIB_DIR)/libCSocket.a
	$(CC) $(LDEBUG) -o $(BIN_DIR)/CSocketTest CSocketINetServer.o $(LIBS)

$(BIN_DIR)/CSocketUnixClient: CSocketUnixClient.o $(LIB_DIR)/libCSocket.a
	$(CC) $(LDEBUG) -o $(BIN_DIR)/CSocketTest CSocketUnixClient.o $(LIBS)

$(BIN_DIR)/CSocketUnixServer: CSocketUnixServer.o $(LIB_DIR)/libCSocket.a
	$(CC) $(LDEBUG) -o $(BIN_DIR)/CSocketTest CSocketUnixServer.o $(LIBS)
