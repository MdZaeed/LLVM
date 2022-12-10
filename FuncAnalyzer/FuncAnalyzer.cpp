/*
 *  Simple LibTool that counts the number of functions in the source file
 */
#include "clang/Basic/SourceManager.h"
#include "clang/Driver/Options.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include<fstream>

using namespace std;
using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
using namespace llvm;

static llvm::cl::OptionCategory MyToolCategory("Function Analyzer options");

unsigned noOfLoops = 0;
unsigned noOfInstructions = 0;

/* 
 * The RecursiveASTVisitor: real magic happens here!
 */
class FuncAnalysisVisitor : public RecursiveASTVisitor<FuncAnalysisVisitor> {
private:
  ASTContext *astContext; 
  
public:
  explicit FuncAnalysisVisitor(CompilerInstance *CI) 
      : astContext(&(CI->getASTContext())) {}
  
  virtual bool VisitFunctionDecl(FunctionDecl *func) {
    if(func->isThisDeclarationADefinition()==0)
    {
	return true;
    }	
    string funcName = func->getQualifiedNameAsString();
    llvm::outs() << "Function name: " << funcName << "\n";
    int numP = func->getNumParams();
    llvm::outs() << "Number of parameters: " << numP << "\n";
//    llvm::outs() << "Definition: " << func->isThisDeclarationADefinition() << "\n";
    llvm::outs() << "Number of Loops: " << noOfLoops << "\n";
    llvm::outs() << "Number of Instruction: " << noOfInstructions << "\n";
    llvm::outs() << "___________________________________________________" << "\n";
    noOfLoops = 0;
    noOfInstructions = 0;
/*    const FunctionDecl* def = func->getDefinition();
    Stmt* bods = func->getBody(def);
    StmtVisitor *vis = new StmtVisitor(astContext); 	
    CompoundStmt* bgs = (CompoundStmt*) bods->getStmtClass();
    if(!bgs) llvm::outs() << "Yes" << "\n";
    else llvm::outs() << "No, is" << "\n";
    unsigned a = 0;
    for(Stmt::const_child_iterator i = bods->child_begin(); i != bods->child_end(); i++)
    {
	    vis->TraverseStmt(*i);
	    a++;
    };
    llvm::outs() << bods->getStmtClassName() << "\n";
    llvm::outs() << "New ins" << a << "\n";
*/
    return true;
  }

  virtual bool VisitStmt(Stmt *stmt) {
    string instructionClass = stmt->getStmtClassName();
	   
    if(instructionClass == "ForStmt" || instructionClass == "DoStmt" || instructionClass =="WhileStmt")
    {
      noOfLoops++;
    }
    
    noOfInstructions++;
    return true;
  }
};


/* 
 * The ASTConsumer: essentially just a wrapper for the Visitors 
 */
class FuncAnalysisASTConsumer : public ASTConsumer {
private:
    FuncAnalysisVisitor *visitor; 

public:

  /* instantiate the visitor and pass the current context */
  explicit FuncAnalysisASTConsumer(CompilerInstance *CI)
    : visitor(new FuncAnalysisVisitor(CI)) { }

  /* override call to HandleTranslationUnit() */
  virtual void HandleTranslationUnit(ASTContext &Context) {

    /* TranslationUnit represents the entire source file (and all expanded headers)
       We can iterate through top-level decls and just select the ones that appear 
       in the main source file 
    */
    SourceManager& SM = Context.getSourceManager();
    auto Decls = Context.getTranslationUnitDecl()->decls();
    for (auto &Decl : Decls) {
      const auto& FileID = SM.getFileID(Decl->getLocation());
      if (FileID != SM.getMainFileID())
     	continue;
      visitor->TraverseDecl(Decl);
    }
  }
};


/* 
 * The FrontEndAction class simply creates an AST Consumer
 *
 * What is the front-end action? 
 *   consume (i.e., process) the AST 
 */
class FuncAnalysisFrontendAction : public ASTFrontendAction {
public:
  virtual std::unique_ptr<ASTConsumer> CreateASTConsumer(clang::CompilerInstance &CI,
							 llvm::StringRef file) {
    return std::unique_ptr<ASTConsumer>(new FuncAnalysisASTConsumer(&CI)); 
  }
};

int main(int argc, const char **argv) {

  // parse the command-line args passed to your code
  CommonOptionsParser op(argc, argv, MyToolCategory);        

  // create a new LibTool instance 
  ClangTool Tool(op.getCompilations(), op.getSourcePathList());

  // run the tool, creating a new FrontendAction
  int result = Tool.run(newFrontendActionFactory<FuncAnalysisFrontendAction>().get());

  return result;
}


