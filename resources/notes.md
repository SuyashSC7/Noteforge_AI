# Exam‑Oriented Study Notes

> *These notes are compiled solely from the files provided. No additional facts or interpretations are added.*

---

## testppt.pptx

- **Key concepts**
  - Slide 1 contains a **ppt key** of 21.
- **Definitions**
  - **ppt key** – the identifier shown in the first slide.
- **Formula / Value**
  ```
  ppt key = 21
  ```
- **Likely exam questions**
  1. What is the value of the ppt key shown on Slide 1?
  2. Which slide contains the ppt key in *testppt.pptx*?

---

## doc.docx

### Key Concepts (bulleted)

- **8259 PIC** – Programmable Interrupt Controller used in 808x/8086 systems.
- **Registers**
  - **IRR** – Interrupt Request Register (pending interrupts).
  - **ISR** – In‑Service Register (currently serviced interrupts).
  - **IMR** – Interrupt Mask Register (mask bits for individual lines).
- **Interrupt modes**
  - Fixed‑priority (IR0 highest, IR7 lowest).
  - Rotating priority.
  - Edge‑triggered vs. level‑triggered.
  - 8085 (MCS‑85) vs. 8086/8088 mode.
- **Cascade & Master/Slave**
  - **CAS** lines (CAS0–CAS2) for slave identification and selection.
  - Cascade mode expands a single PIC to 64 interrupt lines.
- **Command words**
  - **ICW1–ICW4** – Initialization Command Words.
  - **OCW1–OCW3** – Operation Command Words (masking, EOI, status reading).
- **End‑of‑Interrupt (EOI)**
  - Automatic End of Interrupt (AEOI).
  - Specific EOI vs. Non‑specific EOI.
- **Special Modes**
  - Special Fully Nested Mode (SFNM).
  - Buffered mode (SP/EN pin, BUF bit).
  - Special Mask Mode (SMM).

### Definitions (bold)

- **8259 PIC** – Programmable Interrupt Controller.
- **IRR** – Interrupt Request Register.
- **ISR** – In‑Service Register.
- **IMR** – Interrupt Mask Register.
- **CAS** – Cascade lines (CAS0–CAS2).
- **AEOI** – Automatic End of Interrupt.
- **SFNM** – Special Fully Nested Mode.
- **ICW** – Initialization Command Word.
- **OCW** – Operation Command Word.
- **INTA** – Interrupt Acknowledge signal.
- **FIFO** – First In, First Out (implicitly used in buffered mode).
- **BUF** – Buffered mode enable bit.

### Formulas / Assignments (each on its own line)

```
LTIM = 0  →  Edge‑triggered operation
LTIM = 1  →  Level‑triggered operation
IC4 = 0   →  ICW4 not required
IC4 = 1   →  ICW4 required
mPM = 1   →  8086/8088 mode
mPM = 0   →  8085 mode
AEOI = 1  →  Automatic End of Interrupt enabled
SFNM = 1  →  Special Fully Nested Mode enabled
BUF  = 1  →  Buffered mode selected
M/S  = 1  →  Master configuration (when BUF=1)
```

### Likely Exam Questions

1. **Define** the purpose of the **IRR**, **ISR**, and **IMR** in the 8259 PIC.
2. **Explain** how the 8259 PIC selects an interrupt when multiple requests arrive simultaneously.
3. **Differentiate** between **edge‑triggered** and **level‑triggered** operation and indicate which bit controls it.
4. **Describe** the sequence of command words required to initialize a master 8259 in cascade mode.
5. **What** is the effect of setting the **AEOI** bit in ICW4?
6. **Illustrate** the difference between **Specific EOI** and **Non‑specific EOI** commands.
7. **Explain** the role of the **CAS** lines in a cascaded PIC system.
8. **What** does **SFNM** enable in a multi‑cascaded configuration?
9. **List** the conditions under which **ICW4** is not needed during initialization.
10. **Outline** how polling mode operates in the 8259 PIC.

---

## pdftest.pdf

- **Key concepts**  
  The PDF contains only the phrase: *“I like dogs ,cats ,”*.
- **Definitions**  
  None (no technical terminology).
- **Likely exam questions**  
  *No exam‑relevant content present.*

---

## data.txt

- **Key concepts**
  - File contains two spoken instructions:
    1. Ask: “what files do you have?”
    2. Say: “my favourite subject is physics”
- **Definitions**  
  None (plain text).
- **Likely exam questions**  
  *No technical exam content available.*

---