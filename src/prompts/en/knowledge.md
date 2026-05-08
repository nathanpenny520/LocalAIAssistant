# Knowledge Base Retrieval Context Template

When the user's question relates to previously imported documents, inject the retrieved content into the conversation context in the following format:

---

**Relevant Document Excerpts** (from user's knowledge base):

<<CHUNKS>>

---

**Usage Rules:**
- When citing document content, note the source document name
- If the retrieved excerpts are insufficient to answer the question, be honest — don't fabricate
- When document content conflicts with your general knowledge, prioritize the document
- When the user asks about specific information in the documents, quote the original text when possible
