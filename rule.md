---
trigger: always_on
description: "Mandatory engineering rules, architectural constraints, API contracts, and clean architecture standards for backend development."
---

# Universal Backend Engineering Rules & Coding Standards

This ruleset defines persistent architectural constraints, coding standards, and API contracts for backend development. All AI coding assistants (Antigravity, Cursor, Claude, ChatGPT, Copilot) must strictly enforce these invariants across the codebase.

---

## 1. High-Level Architecture

* **Single Deployable Unit:** The backend is a cohesive deployable application.
* **Backend Responsibilities:** The backend exclusively owns authentication, authorization, business rules, persistence, caching, and external provider integrations.
* **Provider Isolation:** Keep vendor-specific SDKs and third-party APIs strictly behind integration interfaces so core business logic never depends on vendor libraries.

---

## 2. Directory Structure (Clean Architecture by Feature)

Organize code primarily **by business feature**, keeping each feature self-contained:

```text
src/main/<language>/<company>/<project>/
├── <feature_name>/                     # e.g., billing, inventory, auth, orders
│   ├── api/
│   │   ├── controller/                 # HTTP controllers (routing only)
│   │   ├── request/                    # Request DTOs / Validation schemas
│   │   └── response/                   # Response DTOs
│   ├── application/                    # Use Cases / Orchestration
│   │   └── <Verb><Noun>UseCase.<ext>   # e.g., CreateInvoiceUseCase
│   ├── domain/                         # Pure business logic & domain entities
│   │   ├── model/                      # Rich domain models
│   │   └── service/                    # Domain services for shared business rules
│   └── infrastructure/                 # External systems & persistence
│       ├── persistence/
│       │   ├── entity/                 # Database entities / ORM models
│       │   ├── repository/             # Data access interfaces & implementations
│       │   └── mapper/                 # Mappers (Entity <-> Domain <-> DTO)
│       └── integration/                # External HTTP/SDK client implementations
│
└── shared/                             # Truly cross-cutting concerns only
    ├── config/                         # App configuration, beans, env setup
    ├── security/                       # Auth filters, token verification
    ├── exception/                      # Global exception handlers & base errors
    └── logging/                        # Logging interceptors & structured logs
```

### Invariants:
1. **Feature Encapsulation:** A feature's controller must **never** call repositories directly.
2. **Infrastructure Containment:** Entities and repositories reside strictly inside `infrastructure/`.
3. **No Business Logic in Shared:** Do not place feature-specific classes in `shared/`.
4. **Inter-Feature Communication:** Features communicate only through public application use-case interfaces, never by querying another feature's internal repository or entities.

---

## 3. Boundary & Layering Invariants

### Controller Rules
**A Controller MAY:**
1. Validate and deserialize the incoming HTTP request.
2. Invoke exactly one application use case.
3. Map the use case result into the standard API response envelope.
4. Return the appropriate HTTP status code.

**A Controller MUST NOT:**
* ❌ Call a repository directly.
* ❌ Instantiate or construct database persistence entities.
* ❌ Perform duplicate validation or execute business logic.
* ❌ Call external third-party providers directly.
* ❌ Read from or write to cache stores.
* ❌ Catch broad generic exceptions (`catch (Exception e)`).
* ❌ Use field injection (e.g., `@Autowired` on private fields); **always use constructor injection**.

### Persistence & Transport Rules
* **Never Expose Entities:** Database entities must **never** be returned from an API endpoint.
* **Explicit DTOs:** Always define explicit Request and Response DTOs at the API boundary.
* **Explicit Mapping:** Use mappers to translate between API DTOs, domain models, and database entities in their respective layers.
* **API Stability:** Database schema modifications must not silently alter the public API contract.

---

## 4. Standard API Response Contract

Every successful HTTP response **must** adhere to this uniform envelope:

```json
{
  "data": {
    "id": "item-123",
    "name": "Example Resource"
  },
  "meta": {
    "requestId": "req-123",
    "timestamp": "2026-10-06T12:00:00Z"
  }
}
```

### Response Rules:
* `"data"` is mandatory for all successful responses (single object, array, or paginated items).
* Pagination metadata belongs inside `"meta"` or a dedicated pagination DTO within `"data"`.
* **Path Naming:** Use resource-oriented, lowercase, plural paths (e.g., `/api/orders`, `/api/users`).
* **Trailing Slashes:** Do **not** add trailing slashes (e.g., use `/api/orders`, not `/api/orders/`).
* **HTTP Status Codes:**
  - `201 Created`: Resource creation (include created resource or ID).
  - `200 OK`: Successful reads, queries, and updates.
  - `204 No Content`: Successful actions with no response body.

---

## 5. Standard Error Response Contract

All failed requests must produce a standard error envelope via a **Global Exception Handler**:

```json
{
  "code": "RESOURCE_NOT_FOUND",
  "message": "The requested resource was not found.",
  "status": 404,
  "timestamp": "2026-10-06T12:00:00Z",
  "traceId": "trace-abc-123",
  "details": []
}
```

### Validation Error Example:
```json
{
  "code": "VALIDATION_FAILED",
  "message": "The request contains invalid fields.",
  "status": 400,
  "timestamp": "2026-10-06T12:00:00Z",
  "traceId": "trace-abc-123",
  "details": [
    {
      "field": "email",
      "message": "Must be a valid email address"
    }
  ]
}
```

### Error Rules:
* `code`: Machine-readable, uppercase snake_case string (e.g., `INVALID_CREDENTIALS`).
* `message`: Safe, user-friendly message. **Never expose SQL errors, raw stack traces, tokens, or internal vendor payloads.**
* `status`: Must match the actual HTTP status code.
* `timestamp`: ISO-8601 UTC format.
* `traceId`: Correlation ID required for debugging and log correlation.
* `details`: Structured array of field-level or sub-error reasons (empty array if none).

---

## 6. Exception Handling Rules

* **Catch Deliberately:** Catch an exception only if the current layer can recover, retry, translate, or enrich context.
* **No Swallowing Exceptions:** Never use `catch (Exception e)` to silently return `null`, `false`, or empty collections.
* **Preserve Cause:** Always preserve the original root cause when wrapping or rethrowing exceptions.
* **Domain Exceptions:** Use strongly-typed domain exceptions for expected business failures (e.g., `InsufficientBalanceException`).
* **Security in Logs:** Never log passwords, tokens, API keys, or full unmasked PII.

---

## 7. Database & Transaction Management

* **Versioned Migrations:** All schema changes must use versioned database migrations (Flyway, Liquibase, Prisma Migrate, etc.). Never edit production tables manually.
* **Transaction Boundary:** Define transaction boundaries at the **Application / Use Case** layer.
* **No External Calls in Transactions:** Never hold a database transaction open while waiting for external HTTP/network calls.
* **Data Integrity:** Enforce uniqueness and referential integrity in both the database schema and application validations.
* **Rollback Testing:** Ensure transactions properly roll back across multi-table operations on failure.

---

## 8. Caching Rules

* **Cache as Optimization:** Treat cache strictly as a performance accelerator, never as the authoritative source of truth.
* **Mandatory TTL:** Every cache entry must have an explicit Time-To-Live (TTL).
* **Write Invalidation:** Mutation operations must actively invalidate or refresh affected cache entries.
* **Namespaced Keys:** Use stable, namespaced keys: `<entity>:<identifier>` (e.g., `user:1049`).
* **Graceful Degradation:** A cache outage must degrade gracefully to the primary database; it must never cause application downtime or invalid data.
* **No Sensitive Data:** Never cache unencrypted passwords, secrets, or sensitive personal data.

---

## 9. External Provider Integrations

* **Encapsulation:** Keep SDKs and HTTP clients inside `infrastructure/integration/`.
* **Timeouts:** Every external call must have an explicit connection and read timeout.
* **Retry Strategy:** Retries must be limited, use exponential backoff with jitter, and apply only to idempotent, retryable errors (e.g., network 503/504).
* **Idempotency Keys:** Use idempotency keys for operations creating payments, notifications, or external side effects.
* **Secret Protection:** Never log authorization headers, credentials, or sensitive payloads.

---

## 10. Logging, Security & Observability

* **Structured Logging:** Use structured fields: `traceId`, `component`, `operation`, `resourceId`.
* **Correlation ID:** Generate a unique correlation/request ID at the gateway/entry point and propagate it across all downstream calls.
* **Secrets Management:** Keep secrets strictly in environment variables or dedicated secret managers—never hardcode in code or config files.
* **Authentication Boundary:** Enforce authentication and resource ownership checks at the API boundary before executing business logic.
* **Health & Metrics:** Provide health check endpoints (`/health`, `/metrics`) covering DB, cache, and external service availability.

---

## 11. Pre-Commit & Pull Request Checklist

Before submitting or generating code, verify:
- [ ] Code follows the standard feature-based directory structure.
- [ ] Controller calls an application use case; does not access repositories or entities directly.
- [ ] Controller returns the unified success response envelope (`data`, `meta`).
- [ ] Failures are handled by the global error envelope (`code`, `message`, `status`, `traceId`, `details`).
- [ ] Constructor injection is used; field injection is eliminated.
- [ ] No broad exceptions (`Exception`, `Throwable`) are swallowed.
- [ ] Database transactions do not wrap external HTTP calls.
- [ ] Cache keys have TTLs and write-invalidation logic.
- [ ] External calls have timeouts and typed failure handling.
- [ ] No secrets, tokens, or raw SQL queries are logged.
