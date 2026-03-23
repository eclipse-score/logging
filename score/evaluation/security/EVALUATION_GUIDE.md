# Security Evaluation Guide

## Scope
- **Source file:** `security_issues.h`
- **Goal:** Verify tool catches common application-security flaws and insecure API patterns.

## Test Conditions and Expected Review Comments
- **SEC-01:** Hardcoded secret/token in source; expected comment recommends secret manager/env injection.
- **SEC-02:** Predictable session ID generation; expected comment recommends cryptographically secure randomness.
- **SEC-03:** Early-exit secret comparison; expected comment recommends constant-time comparison for secrets.
- **SEC-04:** Shell command built from unsanitized input; expected comment flags command-injection risk.
- **SEC-05:** SQL string concatenation with untrusted input; expected comment recommends prepared statements.
- **SEC-06:** User-controlled path concatenation; expected comment flags path traversal and recommends canonicalization/allow-lists.
- **SEC-07:** Password included in log line; expected comment recommends redaction and sensitive-field suppression.
- **SEC-08:** Weak XOR checksum for integrity; expected comment recommends cryptographic MAC/hash depending on threat model.

## Pass Criteria
- Tool should catch security-impactful flaws and provide concrete mitigations appropriate for production hardening.
