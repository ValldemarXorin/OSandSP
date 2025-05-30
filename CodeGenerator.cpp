#include "CodeGenerator.h"
#include "Utils.h"
#include "ErrorHandling.h"
#include <iostream>
#include <mm_malloc.h>
#include <assert.h>

namespace AST
{
    namespace
    {
        class FirstPass : public AstVisitor
        {
        public:
            FirstPass();
            ~FirstPass();

            virtual void Visit(CommandNode* node) override;
            virtual void Visit(OneOperandCommandNode* node) override;
            virtual void Visit(DoubleOperandCommandNode* node) override;

            ProgramNode* GetProgram();
            inline const std::map<std::string, int>& GetLabelsTable() const;

        private:
            void AddInstructionLabels(const CommandNode* node, const int instructionNumber);
            void AddLabel(const char* name, const int instructionNumber);
            unsigned int GetOperandSize(const OperandNode* node, const InstructionGroup g);

        private:
            ProgramNode *              Program;
            unsigned int               CurrentProgramSize;
            std::vector<CommandNode*>  Commands;
            std::map<std::string, int> LabelsTable;
            std::queue<Word>           AdditionalInstructionWords;
        };

        ProgramNode* FirstPass::GetProgram()
        {
            if (Commands.empty() == false)
            {
                CommandNode* n = Commands[0];
                for (size_t i = 1; i < Commands.size(); ++i)
                {
                    n->Next = Commands[i];
                    n = n->Next;
                }

                Program->Commands = Commands[0];
            }

            return Program;
        }

        const std::map<std::string, int>& FirstPass::GetLabelsTable() const
        {
            return LabelsTable;
        }


        FirstPass::FirstPass()
            : Program(new ProgramNode(nullptr))
            , CurrentProgramSize(0)
        {
        }

        FirstPass::~FirstPass()
        {
            delete Program;
        }

        void FirstPass::AddInstructionLabels(const CommandNode* node, const int instructionNumber)
        {
            for (LabelNode* l = node->Labels; l != nullptr; l = l->Next)
                AddLabel(l->Name, instructionNumber);
        }

        void FirstPass::AddLabel(const char* name, const int instructionNumber)
        {
            std::string l(name);
            LabelsTable[l] = instructionNumber;
        }

        void FirstPass::Visit(CommandNode* node)
        {
            const int instructionNumber = CurrentProgramSize++;
            
            AddInstructionLabels(node, instructionNumber);
            
            Commands.push_back(node);
        }

        void FirstPass::Visit(OneOperandCommandNode* node)
        {
            const int instructionNumber = CurrentProgramSize;
            const InstructionGroup g = GetInstructionGroup(node->Opcode);

            CurrentProgramSize += 1 + GetOperandSize(node->First, g);

            Commands.push_back(node);
            AddInstructionLabels(node, instructionNumber);
        }

        void FirstPass::Visit(DoubleOperandCommandNode* node)
        {
            const unsigned int instructionNumber = CurrentProgramSize;
            const InstructionGroup g = GetInstructionGroup(node->Opcode);

            CurrentProgramSize += 1 + GetOperandSize(node->First, g) + GetOperandSize(node->Second, g);

            Commands.push_back(node);
            AddInstructionLabels(node, instructionNumber);
        }

        unsigned int FirstPass::GetOperandSize(const OperandNode* node, const InstructionGroup g)
        {
            unsigned int size = 0;

            if (   node->OpType   == OperandType::Number
                || node->AddrType == AddressingType::Index
                || node->AddrType == AddressingType::IndexDeferred
                || (node->AddrType == AddressingType::Label && g != InstructionGroup::Branch)
               )
            {
                size = 1;
            }
            else
            {
                size = 0;
            }

            return size;
        }

        class SecondPass : public AstVisitor
        {
        public:
            SecondPass(const std::map<std::string, int>& labelsTable, std::vector<Error>& errors);

            virtual void Visit(CommandNode* node) override;
            virtual void Visit(OneOperandCommandNode* node) override;
            virtual void Visit(DoubleOperandCommandNode* node) override;

            inline const std::vector<Word>&& GetProgram() const;
            inline const std::vector<Error>& GetErrors() const;

        private:
            Word GetRawOperand(const OperandNode* node, std::vector<Word>* additionalWords) const;
            Word GetRawLabel(const std::string& labelName, CommandNode* node) const;
            Word ConstructLabelOperand(const Word rawLabel, std::vector<Word>* additionalWords) const;
            Word ConstructDoubleOperand(const OperandNode* opNode, std::vector<Word>* additionalWords, DoubleOperandCommandNode* node) const;
        private:
            const std::map<std::string, int>& LabelsTable;
            std::vector<Word> Program;
            std::vector<Error>& Errors;
        };

        const std::vector<Word>&& SecondPass::GetProgram() const
        {
            return std::move(Program);
        }

        const std::vector<Error>& SecondPass::GetErrors() const
        {
            return Errors;
        }

        SecondPass::SecondPass(const std::map<std::string, int>& labelsTable, std::vector<Error>& errors)
            : LabelsTable(labelsTable)
            , Errors(errors)
        {
        }

        Word SecondPass::GetRawOperand(const OperandNode* node, std::vector<Word>* additionalWords) const
        {
            Word op = 0;

            if (node->OpType == OperandType::Number)
            {
                additionalWords->push_back(node->Value);
                op = RegisterNumber::PC;
            }
            else
            {
                if (node->AddrType == AddressingType::Index || node->AddrType == AddressingType::IndexDeferred)
                    additionalWords->push_back(node->IndexedOffset);

                op = node->Value;
            }

            op |= (static_cast<int>(node->AddrType) << 3);
            return op;
        }

        Word SecondPass::GetRawLabel(const std::string& labelName, CommandNode* node) const
        {
            auto it = LabelsTable.find(labelName);
            if (it != LabelsTable.end())
            {
                return static_cast<uint8_t>(it->second);
            }
            else
            {
                Errors.push_back(Error{ node, std::string("Label doesn't exist:") + labelName });
                return 0;
            }
        }

        Word SecondPass::ConstructLabelOperand(const Word rawLabel, std::vector<Word>* additionalWords) const
        {
            Word op = RegisterNumber::PC;
            op |= (static_cast<int>(AddressingType::AutoIncrement) << 3);
            additionalWords->push_back(rawLabel * sizeof(Word) + GetROMBegining());

            return op;
        }

        Word SecondPass::ConstructDoubleOperand(const OperandNode* opNode, std::vector<Word>* additionalWords, DoubleOperandCommandNode* node) const
        {
            Word op = 0;
            if (opNode->AddrType == AddressingType::Label)
            {
                const Word rawLabel = GetRawLabel(opNode->LabelName, node);
                op = ConstructLabelOperand(rawLabel, additionalWords);
            }
            else
            {
                InstructionGroup g = GetInstructionGroup(node->Opcode);
                op = g == InstructionGroup::OneAndHalf ? GetRawOperand(opNode, additionalWords) & 07 : GetRawOperand(opNode, additionalWords);
            }

            return op;
        }

        void SecondPass::Visit(CommandNode* node)
        {
            Program.push_back(node->Opcode);
        }

        void SecondPass::Visit(OneOperandCommandNode* node)
        {
            std::vector<Word> additionalWords;
            const unsigned int instructionNumber = Program.size();
            Word raw = node->Opcode;

            const OperandNode* first = node->First;
            if (first->AddrType == AddressingType::Label)
            {
                const Word rawLabel = GetRawLabel(first->LabelName, node);
                
                if (GetInstructionGroup(node->Opcode) == InstructionGroup::Branch)
                {
                    const Byte offset = static_cast<Byte>(rawLabel - instructionNumber) - 1;
                    raw |= offset;
                }
                else
                {
                    Word op = ConstructLabelOperand(rawLabel, &additionalWords);
                    raw |= op;
                }
            }
            else
            {
                const Word op = GetRawOperand(first, &additionalWords);
                raw |= op;
            }
            
            Program.push_back(raw);
            if(additionalWords.empty() == false)
                Program.push_back(additionalWords[0]);
        }
    }

    void SecondPass::Visit(DoubleOperandCommandNode* node)
    {
        std::vector<Word> additionalWords;
        const unsigned int instructionNumber = Program.size();
        Word raw = node->Opcode;

        const Word secondOp = ConstructDoubleOperand(node->Second, &additionalWords, node);
        raw |= (secondOp << 6);

        const Word firstOp = ConstructDoubleOperand(node->First, &additionalWords, node);
        raw |= firstOp;

        Program.push_back(raw);
        for (const Word w : additionalWords)
            Program.push_back(w);
    }

    std::vector<Word> CodeGenerator::Generate(AST::AbstractSyntaxTree* ast)
    {
        FirstPass fp;
        ast->Accept(&fp);

        ProgramNode* pn = fp.GetProgram();
        SecondPass sp{ fp.GetLabelsTable(), Errors };
        pn->Accept(&sp);
        std::vector<Word> program = std::move(sp.GetProgram());

        return program;
    }
}
