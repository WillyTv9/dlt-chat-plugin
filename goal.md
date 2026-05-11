# DLT Viewer Plugin – Project Requirements

## 1. Project Overview

### Project Title
DLT Viewer Intelligent Log Analysis Plugin

### Objective
The objective of this project is to develop a plugin for the DLT Viewer application that is capable of reading and analyzing Diagnostic Log and Trace (DLT) files and providing an interactive chat-based interface for users.

The plugin will help users understand complex logs by:
- Reading and parsing logs currently opened in the DLT Viewer application.
- Allowing users to ask questions about the logs.
- Providing contextual explanations of events occurring in the logs.
- Returning references to the relevant log indexes associated with the explanation.

The project is intended for educational purposes and should demonstrate:
- Software architecture design
- Plugin development
- Log analysis
- Basic AI/chat interaction concepts
- User interface integration
- File parsing and indexing

---

# 2. Source Code Repository

## GitLab Repository
The project source code shall be hosted on GitLab.

Official Repository Pointer:

`https://github.com/COVESA/dlt-viewer`


---

# 3. Scope of the Project

## In Scope
The plugin shall:

1. Integrate with the DLT Viewer application.
2. Access and read the currently opened DLT log files.
3. Parse log messages and extract useful information.
4. Provide a graphical chat interface inside the plugin.
5. Allow users to ask natural language questions.
6. Analyze logs based on user requests.
7. Provide contextual explanations about the logs.
8. Return references or indexes of the relevant log entries.
9. Support filtering or searching inside logs.
10. Maintain acceptable performance with large log files.

## Out of Scope
The following items are not required for this project:

- Cloud deployment
- Multi-user collaboration
- Real-time distributed processing
- Full AI model training
- Voice interaction
- Mobile application support
- Modification of original DLT files

---

# 4. Functional Requirements

## 4.1 DLT File Access

### Requirement
The plugin shall access the log files currently opened in the DLT Viewer application.

### Expected Features
- Detect opened DLT files.
- Read log entries.
- Access metadata such as:
  - Timestamp
  - ECU
  - Application ID
  - Context ID
  - Log level
  - Message content

---

## 4.2 Log Parsing and Analysis

### Requirement
The plugin shall analyze log data and identify meaningful information.

### Expected Features
- Parse structured DLT messages.
- Group related logs.
- Identify warnings and errors.
- Detect repetitive patterns.
- Extract context from neighboring log lines.

### Optional Features
- Error categorization
- Timeline reconstruction
- Session grouping
- Keyword extraction

---

## 4.3 Chat-Based User Interface

### Requirement
The plugin shall provide a chat-like interface that allows users to interact with the logs.

### Expected Features
- Text input area
- Chat history display
- User questions and plugin responses
- Scrollable conversation window

### Example Questions
- “What caused the error at timestamp X?”
- “Show all CAN communication failures.”
- “Why did the application restart?”
- “Summarize the critical errors.”

---

## 4.4 Contextual Responses

### Requirement
The plugin shall answer user questions using the context extracted from the logs.

### Expected Features
The response should:
1. Explain what is happening.
2. Describe possible causes.
3. Reference related logs.
4. Provide indexes or identifiers of the relevant log entries.

### Example Response

Question:

> Why did the service restart?

Answer:

> The logs indicate that the service stopped after a communication timeout with ECU_XYZ. After multiple retry failures, the watchdog triggered a restart sequence.
>
> Related log indexes:
> - 1452
> - 1453
> - 1454
> - 1460

---

## 4.5 Log Index Referencing

### Requirement
The plugin shall provide the indexes of the relevant logs used to generate the response.

### Expected Features
- Display log indexes.
- Allow navigation to selected logs.
- Highlight relevant entries inside DLT Viewer.

---

# 5. Non-Functional Requirements

## 5.1 Performance

The plugin should:
- Handle large DLT files efficiently.
- Provide responses within a reasonable time.
- Avoid freezing the DLT Viewer interface.

Target:
- Initial analysis under 10 seconds for medium-sized logs.
- Chat response generation under 5 seconds when possible.

---

## 5.2 Usability

The user interface should:
- Be simple and intuitive.
- Use readable text and layouts.
- Clearly separate user messages and plugin responses.

---

## 5.3 Reliability

The plugin should:
- Handle malformed logs gracefully.
- Avoid application crashes.
- Provide useful error messages.

---

## 5.4 Maintainability

The code should:
- Be modular.
- Use clear naming conventions.
- Include comments and documentation.
- Follow good software engineering practices.

---

# 6. Suggested Technical Architecture

## Components

### 1. DLT Reader Module
Responsible for:
- Reading DLT files
- Extracting log data
- Managing indexes

### 2. Log Analysis Engine
Responsible for:
- Searching logs
- Grouping events
- Building contextual information

### 3. Chat Interface
Responsible for:
- User interaction
- Displaying messages
- Sending queries to the analysis engine

### 4. Response Generator
Responsible for:
- Generating explanations
- Formatting responses
- Returning related indexes

---

# 7. Suggested Technologies

Possible technologies include:

- C++
- Qt Framework
- Python (optional for analysis)
- JSON
- GitLab

Optional AI Integration:
- Local LLM
- OpenAI API
- Rule-based response engine

---

# 8. Deliverables

The students shall provide:

1. Source code repository on GitLab
2. Build instructions
3. Installation guide
4. Technical documentation
5. User manual
6. Demonstration of the plugin
7. Final presentation

---

# 9. Testing Requirements

The project should include:

## Functional Testing
- File reading validation
- Chat interaction testing
- Correct index generation
- Search functionality validation

## Performance Testing
- Large file handling
- Response time measurements

## User Acceptance Testing
- Ease of use
- Correctness of explanations

---

# 10. Acceptance Criteria

The project will be considered successful if:

1. The plugin correctly integrates with DLT Viewer.
2. The plugin can access opened DLT logs.
3. Users can ask questions through the chat interface.
4. The plugin provides meaningful contextual explanations.
5. Relevant log indexes are returned.
6. The application remains stable during operation.
7. Source code and documentation are delivered.

---

# 11. Future Improvements (Optional)

Possible future enhancements:

- AI-powered anomaly detection
- Automatic issue classification
- Exportable reports
- Multi-log correlation
- Real-time log streaming
- Graphical timelines
- Statistics dashboard
- Search suggestions

---

# 12. Conclusion

This project introduces students to practical software engineering concepts through the implementation of a DLT Viewer plugin capable of intelligent log analysis.

The final solution should combine:
- File parsing
- User interaction
- Contextual reasoning
- Software modularity
- Practical debugging assistance

The main educational goal is to create a useful and maintainable tool that improves the understanding of DLT logs through an interactive user experience.
